#include <config.h>
#include <liboaf/liboaf.h>
#include <bonobo.h>

#define TESTSIZE (1024*1)

#define NO_EXCEPTION(ev)		  G_STMT_START{		        \
     if (BONOBO_EX (ev))			       		        \
       g_log (G_LOG_DOMAIN,					        \
	      G_LOG_LEVEL_ERROR,				        \
	      "file %s: line %d: unexpected exception: (%s)",	        \
	      __FILE__,						        \
	      __LINE__,						        \
	      (ev)->_repo_id);		  }G_STMT_END

#define CHECK_EXCEPTION(ev, repo_id)	  G_STMT_START{		        \
     if (!BONOBO_EX (ev) || strcmp ((ev)->_repo_id, repo_id))	        \
       g_log (G_LOG_DOMAIN,					        \
	      G_LOG_LEVEL_ERROR,				        \
	      "file %s: line %d: missing exception %s (got: %s)",	\
	      __FILE__,						        \
	      __LINE__,						        \
	      repo_id,                                                  \
	      (ev)->_repo_id);		                                \
     CORBA_exception_free (ev);           }G_STMT_END


CORBA_ORB orb;

static void
basic_io_tests (CORBA_Object storage, CORBA_Environment *ev)
{
	CORBA_Object stream;
	Bonobo_Stream_iobuf *buf, *obuf;
	gint32 i, j;

	printf ("starting basic tests\n");

	buf = Bonobo_Stream_iobuf__alloc ();
	buf->_buffer = (gpointer)&i;
	buf->_length = 4;
	
	/* try to open non existent file */
	stream = Bonobo_Storage_openStream (storage, "test1.txt",
					    Bonobo_Storage_READ, ev);
	CHECK_EXCEPTION (ev, ex_Bonobo_Storage_NotFound);

	/* create a new file */
	Bonobo_Storage_openStream (storage, "test5.txt",
				   Bonobo_Storage_CREATE, ev);
	NO_EXCEPTION (ev);
	
	/* create a new file */
	stream = Bonobo_Storage_openStream (storage, "test1.txt",
					    Bonobo_Storage_CREATE, ev);
	NO_EXCEPTION (ev);

	/* write test data */
	for (i = 0; i < TESTSIZE; i++) {
		Bonobo_Stream_write (stream, buf, ev);
		NO_EXCEPTION (ev);
	}
	
	bonobo_object_release_unref (stream, ev);
	NO_EXCEPTION (ev);
    	
	/* open an existing file */
	stream = Bonobo_Storage_openStream (storage, "test1.txt",
					    Bonobo_Storage_READ, ev);
	NO_EXCEPTION (ev);
	
	/* write to a read only file */
	Bonobo_Stream_write (stream, buf, ev);
	CHECK_EXCEPTION (ev, ex_Bonobo_Stream_NoPermission);

	/* erase nonexistent file */
	Bonobo_Storage_erase (storage, "t1.txt", ev);
	CHECK_EXCEPTION (ev, ex_Bonobo_Storage_NotFound);

	/* erase the file */
	Bonobo_Storage_erase (storage, "test1.txt", ev);
	NO_EXCEPTION (ev);

	/* compare the content */
	for (j = 0; j < TESTSIZE; j++) {
		Bonobo_Stream_read (stream, 4, &obuf, ev);
		NO_EXCEPTION (ev);
		g_assert (buf->_length == 4); 
		g_assert (*((gint32 *)obuf->_buffer) == j); 
		CORBA_free (obuf);
	}

	/* close the file */
	bonobo_object_release_unref (stream, ev);
	NO_EXCEPTION (ev);

	/* try to open a deleted file */
	stream = Bonobo_Storage_openStream (storage, "test1.txt",
					    Bonobo_Storage_READ, ev);
	CHECK_EXCEPTION (ev, ex_Bonobo_Storage_NotFound);
	
	/* open an existing file */
	stream = Bonobo_Storage_openStream (storage, "test5.txt",
					    Bonobo_Storage_READ, ev);
	NO_EXCEPTION (ev);

	CORBA_exception_free (ev);

	printf ("end basic tests\n");
}

static void
compressed_io_tests (CORBA_Object storage, CORBA_Environment *ev)
{
	CORBA_Object stream;
	Bonobo_Stream_iobuf *buf, *obuf;
	gint32 i, j;

	printf ("starting compressed IO tests\n");

	buf = Bonobo_Stream_iobuf__alloc ();
	buf->_buffer = (gpointer)&i;
	buf->_length = 4;

	/* create a compressed file */
	stream = Bonobo_Storage_openStream (storage, "test2.txt",
					    Bonobo_Storage_CREATE |
					    Bonobo_Storage_COMPRESSED, 
					    ev);
	NO_EXCEPTION (ev);
     
	/* write test data */
	for (j = 0; j < TESTSIZE; j++) {
		i = j/8;
		Bonobo_Stream_write (stream, buf, ev);
		NO_EXCEPTION (ev);
	}

	/* maybe we can´t seek writable compressed files */
	j = Bonobo_Stream_seek (stream, 0, Bonobo_Stream_SEEK_SET, ev);
	if (BONOBO_EX(ev))
		CHECK_EXCEPTION (ev, ex_Bonobo_Stream_NotSupported);

	/* close the file */
	bonobo_object_release_unref (stream, ev);
	NO_EXCEPTION (ev);

	/* open the file again*/
	stream = Bonobo_Storage_openStream (storage, "test2.txt",
					    Bonobo_Storage_READ, ev);
	NO_EXCEPTION (ev);

	/* compare the content */
	for (j = 0; j < TESTSIZE; j++) {
		Bonobo_Stream_read (stream, 4, &obuf, ev);
		NO_EXCEPTION (ev);
		g_assert (buf->_length == 4);
		g_assert (*((gint32 *)obuf->_buffer) == (j/8)); 
		CORBA_free (obuf);
	}

	/* seek to start */
	j = Bonobo_Stream_seek (stream, 0, Bonobo_Stream_SEEK_SET, ev);
	NO_EXCEPTION (ev);
	g_assert (j == 0);

	/* compare the content again*/
	for (j = 0; j < TESTSIZE; j++) {
		Bonobo_Stream_read (stream, 4, &obuf, ev);
		NO_EXCEPTION (ev);
		g_assert (buf->_length == 4);
		g_assert (*((gint32 *)obuf->_buffer) == (j/8)); 
		CORBA_free (obuf);
	}

	/* close the file */
	bonobo_object_release_unref (stream, ev);
	NO_EXCEPTION (ev);

	CORBA_exception_free (ev);

	printf ("end compressed IO tests\n");
}

static void
dir_tests (CORBA_Object storage, CORBA_Environment *ev)
{
	CORBA_Object dir1, dir2;
	gchar buf[1024];
	gint i;

	printf ("starting directory tests\n");

	/* try to open non existent directory */
	dir2 = Bonobo_Storage_openStorage (storage, "testdir",
					   Bonobo_Storage_READ, ev);
	CHECK_EXCEPTION (ev, ex_Bonobo_Storage_NotFound);
	
	/* create a directory */
	dir1 = Bonobo_Storage_openStorage (storage, "testdir",
					   Bonobo_Storage_CREATE, ev);
	NO_EXCEPTION (ev);
	
	/* close the directory */
	bonobo_object_release_unref (dir1, ev);
	NO_EXCEPTION (ev);
	
	/* try to open non existent directory */
	dir2 = Bonobo_Storage_openStorage (storage, "testdir/a",
					   Bonobo_Storage_READ, ev);
	CHECK_EXCEPTION (ev, ex_Bonobo_Storage_NotFound);

	/* create some directories */
	for (i = 0; i < 200; i++) {
		sprintf (buf,"testdir/testdir%d", i);
		dir2 = Bonobo_Storage_openStorage (storage, buf,
						   Bonobo_Storage_CREATE, ev);
		NO_EXCEPTION (ev);
     		bonobo_object_release_unref (dir2, ev);
		NO_EXCEPTION (ev);
	}

	/* erase some directories */
	for (i = 0; i < 100; i++) {
		sprintf (buf,"testdir/testdir%d", i);
		Bonobo_Storage_erase (storage, buf, ev);
		NO_EXCEPTION (ev);
	}
	
	/* try to erase a non empty directory */
	Bonobo_Storage_erase (storage, "testdir", ev);
	CHECK_EXCEPTION (ev, ex_Bonobo_Storage_NotEmpty);

	/* rename the directory */
	Bonobo_Storage_rename (storage, "testdir", "testdir2", ev);
	NO_EXCEPTION (ev);
	
	/* try to remove non existent directories */
	for (i = 0; i < 100; i++) {
		sprintf (buf,"testdir2/testdir%d", i);
		Bonobo_Storage_erase (storage, buf, ev);
		CHECK_EXCEPTION (ev, ex_Bonobo_Storage_NotFound);
	}
	
	/* erase more directories */
	for (i = 100; i < 200; i++) {
		sprintf (buf,"testdir2/testdir%d", i);
		Bonobo_Storage_erase (storage, buf, ev);
		NO_EXCEPTION (ev);
	}
	
	/* try to erase an empty directory */
	Bonobo_Storage_erase (storage, "testdir2", ev);
	NO_EXCEPTION (ev);

	CORBA_exception_free (ev);

	printf ("end directorty tests\n");
}

static void
mime_tests (CORBA_Object storage, CORBA_Environment *ev)
{
	Bonobo_StorageInfo *setinfo, *getinfo;
	CORBA_Object stream;

	printf ("starting mime-type tests\n");

	setinfo = Bonobo_StorageInfo__alloc ();

	/* read the content type */
	getinfo = Bonobo_Storage_getInfo (storage, "", 
					  Bonobo_FIELD_CONTENT_TYPE, ev);
	NO_EXCEPTION (ev);
	g_assert (!strcmp(getinfo->content_type, "x-directory/normal"));
	CORBA_free (getinfo);

	/* set the content type */
	setinfo->content_type = "text/plain";
	Bonobo_Storage_setInfo (storage, "", setinfo, 
				Bonobo_FIELD_CONTENT_TYPE, ev);
	NO_EXCEPTION (ev);

	/* read the content type */
	getinfo = Bonobo_Storage_getInfo (storage, "", 
					  Bonobo_FIELD_CONTENT_TYPE, ev);
	NO_EXCEPTION (ev);
	g_assert (!strcmp(getinfo->content_type, "text/plain"));
	CORBA_free (getinfo);

	/* create a new file */
	stream = Bonobo_Storage_openStream (storage, "mimetest.txt",
					    Bonobo_Storage_CREATE, ev);
	NO_EXCEPTION (ev);

	/* read the content type */
	getinfo = Bonobo_Storage_getInfo (storage, "mimetest.txt", 
					  Bonobo_FIELD_CONTENT_TYPE, ev);
	NO_EXCEPTION (ev);
	g_assert (!strcmp(getinfo->content_type, "application/octet-stream"));
	CORBA_free (getinfo);

	/* close the file */
	bonobo_object_release_unref (stream, ev);
	NO_EXCEPTION (ev);

	/* try to set the content type of a non existent file*/
	setinfo->content_type = "text/xxx-plain";
	Bonobo_Storage_setInfo (storage, "mi.txt", setinfo, 
				Bonobo_FIELD_CONTENT_TYPE, ev);
	CHECK_EXCEPTION (ev, ex_Bonobo_Storage_NotFound);

	/* set the content type */
	setinfo->content_type = "text/xxx-plain";
	Bonobo_Storage_setInfo (storage, "mimetest.txt", setinfo, 
				Bonobo_FIELD_CONTENT_TYPE, ev);
	NO_EXCEPTION (ev);

	/* try to read the content type of a non existent file */
	getinfo = Bonobo_Storage_getInfo (storage, "mi.txt", 
					  Bonobo_FIELD_CONTENT_TYPE, ev);
	CHECK_EXCEPTION (ev, ex_Bonobo_Storage_NotFound);
	
	/* read the content type */
	getinfo = Bonobo_Storage_getInfo (storage, "mimetest.txt", 
					  Bonobo_FIELD_CONTENT_TYPE, ev);
	NO_EXCEPTION (ev);
	g_assert (!strcmp(getinfo->content_type, "text/xxx-plain"));
	CORBA_free (getinfo);

	/* open the file */
	stream = Bonobo_Storage_openStream (storage, "mimetest.txt",
					    Bonobo_Storage_WRITE, ev);
	NO_EXCEPTION (ev);

	/* read the content type */
	getinfo = Bonobo_Stream_getInfo (stream, Bonobo_FIELD_CONTENT_TYPE, 
					 ev);
	NO_EXCEPTION (ev);
	g_assert (!strcmp(getinfo->content_type, "text/xxx-plain"));
	CORBA_free (getinfo);
	
	/* set the content type */
	setinfo->content_type = "text/xxx-yyy";
	Bonobo_Stream_setInfo (stream, setinfo, 
			       Bonobo_FIELD_CONTENT_TYPE, ev);
	NO_EXCEPTION (ev);

	/* read the content type */
	getinfo = Bonobo_Stream_getInfo (stream, Bonobo_FIELD_CONTENT_TYPE, 
					 ev);
	NO_EXCEPTION (ev);
	g_assert (!strcmp(getinfo->content_type, "text/xxx-yyy"));
	CORBA_free (getinfo);
	
	/* close the file */
	bonobo_object_release_unref (stream, ev);
	NO_EXCEPTION (ev);

	/* open the file read only */
	stream = Bonobo_Storage_openStream (storage, "mimetest.txt",
					    Bonobo_Storage_READ, ev);
	NO_EXCEPTION (ev);

	/* try to set the content type on a read only file  */
	setinfo->content_type = "text/xxx-yyy";
	Bonobo_Stream_setInfo (stream, setinfo, 
			       Bonobo_FIELD_CONTENT_TYPE, ev);
	CHECK_EXCEPTION (ev, ex_Bonobo_Stream_NoPermission);	

	/* close the file */
	bonobo_object_release_unref (stream, ev);
	NO_EXCEPTION (ev);

	CORBA_exception_free (ev);

	printf ("end mime-type tests\n");
}

static void
stream_copy_tests (CORBA_Object storage, CORBA_Environment *ev)
{
	CORBA_Object stream;
	Bonobo_StorageInfo *setinfo, *getinfo;
	Bonobo_Stream_iobuf *buf, *obuf;
	gint32 i, j;
	CORBA_long bread, bwritten;

	printf ("starting copy tests\n");

	setinfo = Bonobo_StorageInfo__alloc ();

	buf = Bonobo_Stream_iobuf__alloc ();
	buf->_buffer = (gpointer)&i;
	buf->_length = 4;

	/* create a new file */
	stream = Bonobo_Storage_openStream (storage, "copy1.txt",
					    Bonobo_Storage_CREATE, ev);
	NO_EXCEPTION (ev);

	/* set the content type */
	setinfo->content_type = "text/plain";
	Bonobo_Stream_setInfo (stream, setinfo, 
			       Bonobo_FIELD_CONTENT_TYPE, ev);
	NO_EXCEPTION (ev);

	/* check the content type */
	getinfo = Bonobo_Stream_getInfo (stream, Bonobo_FIELD_CONTENT_TYPE, 
					 ev);
	NO_EXCEPTION (ev);
	g_assert (!strcmp(getinfo->content_type, "text/plain"));
	CORBA_free (getinfo);

	/* write test data */
	for (i = 0; i < TESTSIZE; i++) {
		Bonobo_Stream_write (stream, buf, ev);
		NO_EXCEPTION (ev);
	}
	
	/* seek to start of file */
	Bonobo_Stream_seek (stream, 0, Bonobo_Stream_SEEK_SET, ev);
	NO_EXCEPTION (ev);

	/* copy the whole file */
	Bonobo_Stream_copyTo (stream, "copy2.txt", -1, &bread, &bwritten, ev);
	NO_EXCEPTION (ev);
	
	g_assert (bread == (TESTSIZE*4));
	g_assert (bwritten == (TESTSIZE*4));

	/* close the file */
	bonobo_object_release_unref (stream, ev);
	NO_EXCEPTION (ev);

	/* open the copied file */
	stream = Bonobo_Storage_openStream (storage, "copy2.txt",
					    Bonobo_Storage_READ, ev);
	NO_EXCEPTION (ev);

	/* compare the content */
	for (j = 0; j < TESTSIZE; j++) {
		Bonobo_Stream_read (stream, 4, &obuf, ev);
		NO_EXCEPTION (ev);
		g_assert (buf->_length == 4); 
		g_assert (*((gint32 *)obuf->_buffer) == j); 
		CORBA_free (obuf);
	}

	/* check the content type */
	getinfo = Bonobo_Stream_getInfo (stream, Bonobo_FIELD_CONTENT_TYPE, 
					 ev);
	NO_EXCEPTION (ev);
	g_assert (!strcmp(getinfo->content_type, "text/plain"));
	CORBA_free (getinfo);

	/* close the file */
	bonobo_object_release_unref (stream, ev);
	NO_EXCEPTION (ev);

	CORBA_exception_free (ev);

	printf ("end copy tests\n");
}

int
main (int argc, char *argv [])
{
	BonoboStorage *bonobo_storage1, *bonobo_storage2;
	CORBA_Environment ev;
	CORBA_Object storage1, storage2;
	gchar *driver_list[] = { "efs", "fs", NULL };
	gchar *driver;
	gint dn = 0;

	CORBA_exception_init (&ev);

	
        gnome_init_with_popt_table ("MyShell", "1.0",
				    argc, argv,
				    oaf_popt_options, 0, NULL); 
	orb = oaf_init (argc, argv);
	
	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error (_("Can not bonobo_init"));

	while ((driver = driver_list [dn++])) {
		printf ("TEST DRIVER: %s\n", driver);
		
		system ("rm -rf /tmp/storagetest.dir1");
		system ("rm -rf /tmp/storagetest.dir2");
		
		CORBA_exception_free (&ev); 
		
		bonobo_storage1 = bonobo_storage_open_full 
			(driver, "/tmp/storagetest.dir1", 
			 Bonobo_Storage_WRITE | 
			 Bonobo_Storage_CREATE, 0664, &ev);
		NO_EXCEPTION (&ev);

		storage1 = BONOBO_OBJECT(bonobo_storage1)->corba_objref;
		g_assert (storage1 != NULL);
  
		basic_io_tests (storage1, &ev);
		compressed_io_tests (storage1, &ev);
		dir_tests (storage1, &ev);

		if (strcmp (driver, "fs")) { /* fs driver does not work */ 
			mime_tests (storage1, &ev);
			stream_copy_tests (storage1, &ev);

			Bonobo_Storage_commit (storage1, &ev);
			NO_EXCEPTION (&ev);

			bonobo_storage2 = bonobo_storage_open_full 
				(driver, "/tmp/storagetest.dir2", 
				 Bonobo_Storage_WRITE | 
				 Bonobo_Storage_CREATE, 0664, &ev);
			NO_EXCEPTION (&ev);

			storage2 = BONOBO_OBJECT(bonobo_storage2)->corba_objref;
			g_assert (storage2 != NULL);

			bonobo_storage_copy_to (storage1, storage2, &ev);
			NO_EXCEPTION (&ev);

			Bonobo_Storage_commit (storage2, &ev);
			NO_EXCEPTION (&ev);
		
			bonobo_object_unref (BONOBO_OBJECT(bonobo_storage2));
		}

		bonobo_object_unref (BONOBO_OBJECT(bonobo_storage1));

	}

	CORBA_exception_free (&ev);
   
	return 0;
}

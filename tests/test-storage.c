#include <config.h>
#include <gnome.h>
#include <liboaf/liboaf.h>

#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <bonobo/bonobo.h>

CORBA_Environment ev;
CORBA_ORB orb;

int
main (int argc, char *argv [])
{
	BonoboStorage *storage;
	CORBA_Object o;
	CORBA_Object s;
	char *file;
	
	if (argc != 2){
		fprintf (stderr, "Usage is: test-storage storage-file-to-create\n");
		exit (1);
	}

	file = argv [1] ;
	
	CORBA_exception_init (&ev);
	
        gnome_init_with_popt_table("MyShell", "1.0",
				   argc, argv,
				   oaf_popt_options, 0, NULL); 
	orb = oaf_init (argc, argv);
	
	if (bonobo_init (orb, NULL, NULL) == FALSE)
		g_error (_("Can not bonobo_init"));

	unlink (file);
	storage = bonobo_storage_open("efs", file, BONOBO_SS_RDWR|BONOBO_SS_CREATE,0664);

	if (storage == NULL)
		g_error ("Could not create storage file %s", file);
	
	o = BONOBO_OBJECT(storage)->corba_objref;
	printf("Storage %p\n",o);
  
	s = Bonobo_Storage_create_storage(o,"testdir1",&ev);
	printf("CORBA STORAGE %p\n",s);

	s = Bonobo_Storage_create_storage(s,"subdir1",&ev);
	printf("CORBA STORAGE %p\n",s);

	s = Bonobo_Storage_create_storage(o,"testdir1",&ev);
	printf("CORBA STORAGE %p\n",s);
 
	Bonobo_Storage_commit(o,&ev);

	s = Bonobo_Storage_open_storage(o,"testdir1",Bonobo_Storage_READ,&ev);
	printf("CORBA STORAGE %p\n",s);

	s = Bonobo_Storage_open_storage(s,"subdir1",Bonobo_Storage_READ,&ev);
	printf("CORBA STORAGE %p\n",s);
  

  
	{
		Bonobo_Storage_directory_list *list;
		int i;

		list = Bonobo_Storage_list_contents(o,"/",&ev);
		printf("DIRLIST %p\n",list);

		if (list) {
			for (i=0;i<list->_length;i++) {
				printf("DIR: %s\n",list->_buffer[i]);
			}
			CORBA_free(list);
		}
	}
  
 

	{
		Bonobo_Stream_iobuf *buf;
		CORBA_long i;
		buf = Bonobo_Stream_iobuf__alloc ();
		buf->_buffer = CORBA_sequence_CORBA_octet_allocbuf (1000);
		strcpy(buf->_buffer,"This is a Test\n");
		buf->_length = strlen(buf->_buffer);

		s = Bonobo_Storage_create_stream(o,"t.txt",&ev);
		printf("CORBA STREAM %p\n",s);

		i = Bonobo_Stream_write(s, buf, &ev);
		printf("Written: %d\n",i);

		//Bonobo_Stream_close(s, &ev);

	}
    
	CORBA_exception_free (&ev);

	return 0;
}

#include <config.h>
#include <gnome.h>
#include <libgnorba/gnorba.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>
#include <bonobo/gnome-bonobo.h>

CORBA_Environment ev;
CORBA_ORB orb;

char *server_id;

void
main (int argc, char *argv [])
{
  GnomeStorage *storage;
  CORBA_Object o;
  CORBA_Object s;

  if (argc != 1){
    server_id = argv [1];
  }
	
  CORBA_exception_init (&ev);
	
  gnome_CORBA_init ("MyShell", "1.0", &argc, argv, 0, &ev);
  orb = gnome_CORBA_ORB ();
	
  if (bonobo_init (orb, NULL, NULL) == FALSE)
    g_error (_("Can not bonobo_init\n"));

  system("rm -rf /tmp/testdb");
  storage = gnome_storage_open("efs", "/tmp/testdb",GNOME_SS_RDWR|GNOME_SS_CREATE,0664);

  o = GNOME_OBJECT(storage)->corba_objref;
  printf("Storage %p\n",o);
  
  s = GNOME_Storage_create_storage(o,"testdir1",&ev);
  printf("CORBA STORAGE %p\n",s);

  s = GNOME_Storage_create_storage(s,"subdir1",&ev);
  printf("CORBA STORAGE %p\n",s);

  s = GNOME_Storage_create_storage(o,"testdir1",&ev);
  printf("CORBA STORAGE %p\n",s);
 
  GNOME_Storage_commit(o,&ev);

  s = GNOME_Storage_open_storage(o,"testdir1",GNOME_Storage_READ,&ev);
  printf("CORBA STORAGE %p\n",s);

  s = GNOME_Storage_open_storage(s,"subdir1",GNOME_Storage_READ,&ev);
  printf("CORBA STORAGE %p\n",s);
  

  
  {
    GNOME_Storage_directory_list *list;
    int i;

    list = GNOME_Storage_list_contents(o,"/",&ev);
    printf("DIRLIST %p\n",list);

    if (list) {
	    for (i=0;i<list->_length;i++) {
		    printf("DIR: %s\n",list->_buffer[i]);
	    }
	    CORBA_free(list);
    }
  }
  
 

  {
    GNOME_Stream_iobuf *buf;
    CORBA_long i;
    buf = GNOME_Stream_iobuf__alloc ();
    buf->_buffer = CORBA_sequence_CORBA_octet_allocbuf (1000);
    strcpy(buf->_buffer,"This is a Test\n");
    buf->_length = strlen(buf->_buffer);

    s = GNOME_Storage_create_stream(o,"t.txt",&ev);
    printf("CORBA STREAM %p\n",s);

    i = GNOME_Stream_write(s, buf, &ev);
    printf("Written: %d\n",i);

    //GNOME_Stream_close(s, &ev);

  }
    
  CORBA_exception_free (&ev);
}

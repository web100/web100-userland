
#include <stdlib.h>
#include <stdio.h> 
#include <gtk/gtk.h>

//#include "web100.h"
//#include "web100obj.h"
//#include "web100gui.h" 
#include "sockdata.h"
#include "cnlst_wgt.h"


#define CNLST_DEFAULT_SIZE 300

static void cnlst_class_init (CnlstClass *class);
static void cnlst_init (Cnlst *cnlst);
static void cnlst_destroy (GtkObject *object);
static void list_callback(struct SockData *, void *);
static void choose_sort(GtkWidget *, gpointer);
static void row_destroy (gpointer data);

static GtkVBoxClass *parent_class = NULL;

static char *states[12] = {
    "ESTBLSH",   "SYNSENT",   "SYNRECV",   "FWAIT1",   "FWAIT2", "TMEWAIT",
    "CLOSED",    "CLSWAIT",   "LASTACK",   "LISTEN", "CLOSING" };

static char *text[8];
static guint update_id;
static guint update_interval = 1;
int (*sort_compar)(const void *, const void *);

GtkType cnlst_get_type ()
{
  static guint cnlst_type = 0;

  if(!cnlst_type)
    {
      GtkTypeInfo cnlst_info =
      {
	"Cnlst",
	sizeof (Cnlst),
	sizeof (CnlstClass),
	(GtkClassInitFunc) cnlst_class_init,
	(GtkObjectInitFunc) cnlst_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL
      };

      cnlst_type = gtk_type_unique (gtk_vbox_get_type (), &cnlst_info);
    }

  return cnlst_type;
}

static void cnlst_class_init (CnlstClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = cnlst_destroy;
}

static void cnlst_init (Cnlst *cnlst)
{ 
  gchar *titles[8] = { "cmdline", "pid", "local add", "local port",
                       "remote add", "remote port", "cid", "state" };
  GtkWidget *scrolladj, *scrollwin, *hbox, *label, *button[5];      
  char *labels[5] = { "cmdline", "local port", "remote add", "remote port", "cid"};
  int ii;

  GTK_WIDGET_SET_FLAGS (cnlst, GTK_NO_WINDOW); 

  for(ii=0;ii<8;ii++) if( (text[ii] = malloc(60))==NULL) {
    fprintf(stderr, "Out of memory!\n");
    exit(1);
  }

  sort_compar = NULL;

  scrollwin = gtk_scrolled_window_new(NULL, NULL); 
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS); 
  gtk_box_pack_start(GTK_BOX(cnlst), scrollwin, TRUE, TRUE, 0);
  gtk_widget_show(scrollwin);

  cnlst->clist = gtk_clist_new_with_titles(8, titles);

  gtk_widget_set_usize (GTK_WIDGET( cnlst->clist), 500, 250);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 0, 50);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 1, 30);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 2, 80);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 3, 50);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 4, 80);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 5, 50);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 6, 30);
  gtk_clist_set_column_width(GTK_CLIST(cnlst->clist), 7, 60);
  gtk_container_add(GTK_CONTAINER(scrollwin), cnlst->clist);
  gtk_widget_show(cnlst->clist);

  update_id = gtk_timeout_add(update_interval*1000,
                              (GtkFunction) cnlst_update, (gpointer) cnlst);
  cnlst_update((gpointer) cnlst); 

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(cnlst), hbox, FALSE, FALSE, 10);
  gtk_widget_show(hbox);

  label = gtk_label_new("Sort entries by:  ");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 10);
  gtk_widget_show(label);

  button[0] = gtk_radio_button_new_with_label (NULL, labels[0]);
  gtk_signal_connect(GTK_OBJECT(button[0]), "pressed",
                     GTK_SIGNAL_FUNC(choose_sort), GINT_TO_POINTER(0));

  gtk_box_pack_start(GTK_BOX(hbox), button[0], TRUE, FALSE, 10);
  gtk_widget_show(button[0]);

  for (ii=1;ii<5;ii++) {
  button[ii] =
    gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button[ii-1]),
                                                labels[ii]);
  gtk_signal_connect(GTK_OBJECT(button[ii]), "pressed",
                     GTK_SIGNAL_FUNC(choose_sort), GINT_TO_POINTER(ii));
  gtk_box_pack_start(GTK_BOX(hbox), button[ii], TRUE, FALSE, 10);
  gtk_widget_show(button[ii]);
  }
}

GtkWidget* cnlst_new (void)
{
  Cnlst *cnlst = gtk_type_new (cnlst_get_type ()); 

  return GTK_WIDGET (cnlst); 
}

gint cnlst_update (gpointer data)
{ 
  Cnlst *cnlst = (Cnlst *) data;
  int jj; 
  GtkCList *clist;
  float vadjust;

  clist = GTK_CLIST(cnlst->clist);

  fill_socklist(); 

  gtk_clist_freeze(GTK_CLIST(clist));

  vadjust = GTK_CLIST(clist)->vadjustment->value;

  gtk_clist_clear(GTK_CLIST(clist) );

  sort_socklist(sort_compar);
  list_socklist(list_callback, cnlst);

  gtk_adjustment_set_value( GTK_ADJUSTMENT(GTK_CLIST(clist)->vadjustment), vadjust);

  gtk_clist_thaw(GTK_CLIST(clist)); 

  free_socklist();

  return TRUE;
}

static void cnlst_destroy (GtkObject *object)
{
  Cnlst *cnlst;

  g_return_if_fail (object != NULL);
  g_return_if_fail (IS_CNLST(object)); 
/*
  cnlst = CNLST (object);

  if (cnlst->clist)
    gtk_widget_destroy (cnlst->clist);
*/
  gtk_timeout_remove(update_id);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

void list_callback(struct SockData *socklist, void *data)
{
  Cnlst *cnlst = (Cnlst *) data;
  int ii=0; 

  strcpy(text[0], socklist->cmdline); 
  sprintf(text[1], "%d", socklist->pid);
  sprintf(text[2], "%d.%d.%d.%d", (socklist->localadd)&0xff,
      ((socklist->localadd)>>8)&0xff,
      ((socklist->localadd)>>16)&0xff,
      ((socklist->localadd)>>24)&0xff);
  sprintf(text[3], "%u", socklist->localport);
  sprintf(text[4], "%d.%d.%d.%d", (socklist->remoteadd)&0xff,
      ((socklist->remoteadd)>>8)&0xff,
      ((socklist->remoteadd)>>16)&0xff,
      ((socklist->remoteadd)>>24)&0xff); 
  sprintf(text[5], "%u", socklist->remoteport); 
  strcpy(text[6], socklist->cid); 
  if(socklist->state) strcpy(text[7], states[socklist->state - 1]);
  else strcpy(text[7], "___");

//  gtk_clist_append(GTK_CLIST(cnlst->clist), text); 
  gtk_clist_insert (GTK_CLIST (cnlst->clist), ii, text);
  gtk_clist_set_row_data_full (GTK_CLIST (cnlst->clist), ii, &socklist->cid,
                               (GtkDestroyNotify) row_destroy);
}

// utility functions for sorting list
int cmd_compar(const void *data1, const void *data2)
{
  strcmp(((struct SockData *) data1)->cmdline,
         ((struct SockData *) data2)->cmdline);
}

int lp_compar(const void *data1, const void *data2)
{
  unsigned int add1 = ((struct SockData *) data1)->localport;
  unsigned int add2 = ((struct SockData *) data2)->localport;

  if(add2 > add1) return -1;
  if(add1 > add2) return 1;
  return 0;
}

int ra_compar(const void *data1, const void *data2)
{
// lexicographically
  unsigned int add1 = (((struct SockData *) data1)->remoteadd)&0xff;
  unsigned int add2 = (((struct SockData *) data2)->remoteadd)&0xff;

  if(add2 > add1) return -1;
  if(add1 > add2) return 1;

  add1 = (((struct SockData *) data1)->remoteadd >>8)&0xff;
  add2 = (((struct SockData *) data2)->remoteadd >>8)&0xff;

  if(add2 > add1) return -1;
  if(add1 > add2) return 1;

  add1 = (((struct SockData *) data1)->remoteadd >>16)&0xff;
  add2 = (((struct SockData *) data2)->remoteadd >>16)&0xff;

  if(add2 > add1) return -1;
  if(add1 > add2) return 1;

  add1 = (((struct SockData *) data1)->remoteadd >>24)&0xff;
  add2 = (((struct SockData *) data2)->remoteadd >>24)&0xff;

  if(add2 > add1) return -1;
  if(add1 > add2) return 1;
  return 0;
}

int rp_compar(const void *data1, const void *data2)
{
  unsigned int add1 = ((struct SockData *) data1)->remoteport;
  unsigned int add2 = ((struct SockData *) data2)->remoteport;

  if(add2 > add1) return -1;
  if(add1 > add2) return 1;
  return 0;
}

void choose_sort(GtkWidget *button, gpointer data)
{ 
  int ii = GPOINTER_TO_INT(data);

  switch(ii)
  {
    case 0:
      sort_compar = cmd_compar;
      printf("0\n");
      break;
    case 1:
      sort_compar = lp_compar;
      printf("1\n");
      break;
    case 2:
      sort_compar = ra_compar;
      printf("2\n");
      break;
    case 3:
      sort_compar = rp_compar;
      printf("3\n");
      break;
    case 4:
      sort_compar = NULL;
      printf("4\n");
      break;
    default:
      break;
  }
}

static void row_destroy (gpointer data)
{
//  free (data);
}

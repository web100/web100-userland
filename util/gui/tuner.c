
/*
 *  Copyrights
 *
 *   All documentation and programs in this release is copyright (c)
 *   Carnegie Mellon University, The Board of Trustees of the University of
 *   Illinois, and University Corporation for Atmospheric Research, 2001.
 *   This software comes with NO WARRANTY.
 *
 *   The kernel changes and additions are also covered by the GPL version 2.
 *
 *   Since our code is currently under active development we prefer that
 *   everyone gets the it directly from us.  This will permit us to
 *   collaborate with all of the users.  So for the time being, please refer
 *   potential users to us instead of redistributing web100.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include "web100.h"
#include "web100gui.h"

enum web100_stuning_mode {
  STUNEMODE_DEFAULT = 0,
  STUNEMODE_SETSOCKOPT,
  STUNEMODE_FIXED,
  STUNEMODE_AUTO,
  STUNEMODE_EXP1,
  STUNEMODE_EXP2
};

enum web100_rtuning_mode {
  RTUNEMODE_DEFAULT = 0,
  RTUNEMODE_SETSOCKOPT,
  RTUNEMODE_FIXED,
  RTUNEMODE_AUTO,
  RTUNEMODE_EXP1,
  RTUNEMODE_EXP2
};

struct web100_agent *ag;
struct web100_connection *conn;
struct web100_var *amf;
char cid[20];
char hostname[60];
char ftuple[60];
char optchar[3];

struct tunedata{
  struct web100_var *wv;
  float scale_val;
  unsigned int buf_val;
} *tdata;

int modality=0;
gboolean preset[2];
float preset_val[2];

GtkWidget *topwin, *hostbox, *sessbox, *scalebox, *buttonbox, *psetbox, *setbox;
GtkAdjustment *adjust;
GtkWidget *title,
	  *scale,
	  *label, 
	  *mode[3],
	  *check[2],
	  *setbutton;
GtkStyle *rcstyle;
GdkFont *font;
gint hsize, vsize;

float factory_default = 0.313780; // log-scales (with shift) to 2^16-1
char buf_val_text[20];
char titles[20]; 
int windex[4] = { 0, 1, 2, 3 }; 
char text[4][20];

//#define SAUTOMODE_FIXED   1 
//#define RAUTOMODE_FIXED   1

void value_changed(GtkAdjustment *adj, gpointer data)
{
  float ll = log(4000000)-log(10000), aa = log(10000);

  tdata->scale_val = adj->value;
  tdata->buf_val = floor(exp(ll*(adj->value) + aa));
  sprintf(buf_val_text, "%u", (tdata->buf_val)); 
  gtk_label_set_text(GTK_LABEL(label), buf_val_text);
}

void set_buf_val(GtkWidget *button, gpointer data)
{ 
  u_int32_t mode;
  
  web100_raw_put_any(conn, tdata->wv, &tdata->buf_val, 4 /* size of buf_val */);
#ifdef RECEIVE
  mode = RTUNEMODE_FIXED;
#else
  mode = STUNEMODE_FIXED;
#endif
  web100_raw_put_any(conn, amf, &mode, sizeof mode);
}

void mode_selected(GtkWidget *button, gpointer data)
{
  int ii = *(int *) data;

  GTK_WIDGET_SET_FLAGS(scale, GTK_SENSITIVE);

  if(ii>=1){
    if(preset[ii-1]){
      gtk_adjustment_set_value(adjust, preset_val[ii-1]);
      gtk_adjustment_value_changed(adjust); 
      GTK_WIDGET_UNSET_FLAGS(scale, GTK_SENSITIVE);
    }
  } else {
    gtk_adjustment_set_value(adjust, factory_default); 
    gtk_adjustment_value_changed(adjust);
    gtk_adjustment_value_changed(adjust); 
  } 
  modality = ii;
}

void set_preset(GtkWidget *button, gpointer data)
{
  int ii = *(int *) data;
  preset[ii] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
  if(preset[ii]) preset_val[ii] = tdata->scale_val; 
  else GTK_WIDGET_SET_FLAGS(scale, GTK_SENSITIVE);
}

void data_init()
{
  int ii;

  tdata = malloc(sizeof(struct tunedata));
  tdata->scale_val = factory_default; 
#ifdef RECEIVE
  tdata->wv=web100_find_var(conn, "RcvbufSet"); 
  amf = web100_find_var(conn, "RTuneMode");
#else
  tdata->wv=web100_find_var(conn, "SndbufSet");
  amf = web100_find_var(conn, "STuneMode");
#endif
  for(ii=0;ii<2;ii++) preset_val[ii] = factory_default;
}

void define_sessbox(GtkWidget *hbox)
{ 
  GtkWidget *hbox1, *frame, *entry; 
  void *val;
  int jj=0;

  if(conn->error){
    web100_perror(conn);
    return;
  }
  sprintftuple(ftuple, conn);

  hbox1 = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), hbox1, TRUE, FALSE, 0);
  gtk_widget_show(hbox1);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_label(GTK_FRAME(frame), "TCP session name");
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.0); 
  gtk_box_pack_start(GTK_BOX(hbox1), frame, FALSE, FALSE, 0);
  gtk_widget_show(frame);

  entry = gtk_entry_new(); 
  gtk_entry_set_editable(GTK_ENTRY(entry), FALSE); 
  hsize = gdk_string_width(font, ftuple);
  hsize += gdk_string_width(font, "w");
  vsize = gdk_string_height(font, ftuple);
  gtk_widget_set_usize(entry, hsize, 1.6*vsize);
  gtk_entry_set_text(GTK_ENTRY(entry), ftuple); 
  gtk_container_add(GTK_CONTAINER(frame), entry);
  gtk_widget_show(entry);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_label(GTK_FRAME(frame), "CID");
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.0); 
  gtk_box_pack_start(GTK_BOX(hbox1), frame, FALSE, FALSE, 0);
  gtk_widget_show(frame);

  entry = gtk_entry_new(); 
  gtk_entry_set_editable(GTK_ENTRY(entry), FALSE);
  hsize = gdk_string_width(font, conn->cid);
  hsize += gdk_string_width(font, "w");
  gtk_widget_set_usize(entry, hsize, 1.6*vsize);
  gtk_entry_set_text(GTK_ENTRY(entry), cid); 
  gtk_container_add(GTK_CONTAINER(frame), entry);
  gtk_widget_show(entry);
}

void define_scalebox(GtkWidget *vbox)
{ 
  int ii;

  title = gtk_label_new(titles);
  gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
  gtk_widget_show(title);

  adjust = GTK_ADJUSTMENT(gtk_adjustment_new(tdata->scale_val,
	0.0, 1.0, 0.01, 0.1, 0.0)); 
  gtk_signal_connect(GTK_OBJECT(adjust), "value_changed",
                     (GtkSignalFunc) value_changed, NULL);
  scale = gtk_hscale_new(adjust);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE); 
  gtk_scale_set_digits(GTK_SCALE(scale), 10); 
  gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show(scale);

  label = gtk_label_new(NULL);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5); 
  gtk_widget_show(label);

  gtk_signal_emit_by_name(GTK_OBJECT(adjust), "value_changed"); 
}

void define_buttonbox(GtkWidget *hbox)
{
  GtkWidget *vbox; 
  int ii;

  mode[0] = gtk_button_new_with_label("Factory"); 
  mode[1] = gtk_button_new_with_label("Preset1"); 
  mode[2] = gtk_button_new_with_label("Preset2"); 
 
  for(ii=0;ii<3;ii++){
    gtk_signal_connect(GTK_OBJECT(mode[ii]), "clicked",
	GTK_SIGNAL_FUNC(mode_selected), &windex[ii]);
    gtk_box_pack_start(GTK_BOX(hbox), mode[ii], TRUE, FALSE, 10);
    gtk_widget_show(mode[ii]);
  }
}

void define_psetbox(GtkWidget *hbox)
{
  int ii;

  check[0] = gtk_check_button_new_with_label("lock 'Preset1' value");
  GTK_WIDGET_UNSET_FLAGS(check[0], GTK_CAN_FOCUS);
  gtk_signal_connect(GTK_OBJECT(check[0]), "clicked",
                     GTK_SIGNAL_FUNC(set_preset), &windex[0]);
  gtk_box_pack_start(GTK_BOX(hbox), check[0], FALSE, FALSE, 0);
  gtk_widget_show(check[0]);

  check[1] = gtk_check_button_new_with_label("lock 'Preset2' value");
  GTK_WIDGET_UNSET_FLAGS(check[1], GTK_CAN_FOCUS);
  gtk_signal_connect(GTK_OBJECT(check[1]), "clicked",
                     GTK_SIGNAL_FUNC(set_preset), &windex[1]); 
  gtk_box_pack_start(GTK_BOX(hbox), check[1], FALSE, FALSE, 0);
  gtk_widget_show(check[1]);
}

void define_setbox(GtkWidget *hbox)
{
  setbutton = gtk_button_new_with_label(" Write buf ");
  gtk_signal_connect(GTK_OBJECT(setbutton), "clicked",
      GTK_SIGNAL_FUNC(set_buf_val), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), setbutton, TRUE, FALSE, 0);
  gtk_widget_show(setbutton);
}

void define_topwin()
{ 
  GtkWidget *vbox, *sep, *hbox, *button;

  gtk_signal_connect (GTK_OBJECT (topwin), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(topwin), vbox);
  gtk_widget_show(vbox);

  sessbox = gtk_hbox_new(TRUE, 0);
  define_sessbox(sessbox);
  gtk_box_pack_start(GTK_BOX(vbox), sessbox, FALSE, FALSE, 0);
  gtk_widget_show(sessbox);

  scalebox = gtk_vbox_new(FALSE, 0);
  define_scalebox(scalebox);
  gtk_box_pack_start(GTK_BOX(vbox), scalebox, FALSE, FALSE, 0);
  gtk_widget_show(scalebox);

  sep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);
  gtk_widget_show(sep);

  buttonbox = gtk_hbox_new(FALSE, 0);
  define_buttonbox(buttonbox);
  gtk_box_pack_start(GTK_BOX(vbox), buttonbox, FALSE, FALSE, 5);
  gtk_widget_show(buttonbox);

  setbox = gtk_hbox_new(FALSE, 0);
  define_setbox(setbox);
  gtk_box_pack_start(GTK_BOX(vbox), setbox, FALSE, FALSE, 5);
  gtk_widget_show(setbox);

  psetbox = gtk_hbox_new(FALSE, 0);
  define_psetbox(psetbox);
  gtk_box_pack_start(GTK_BOX(vbox), psetbox, FALSE, FALSE, 0);
  gtk_widget_show(psetbox);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
  gtk_widget_show(hbox);

  button = gtk_button_new_with_label(" Close ");
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 3);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
}

int main(int argc, char *argv[])
{ 
  char titlebar[65]; 
  char temp[3];
  int option;

  gtk_init(&argc, &argv);

  if (argc < 2) {
    printf("Usage: stuner ConnectionID [-l(ocal)|-r(emote)]\n");
    exit(2);
  } else strcpy(cid, argv[1]);

  option = getopt(argc, argv, "lr");
  strcpy(optchar, "-");
  switch(option)
  {
    case 'l':
      gtk_rc_parse("lcl.rc");
      sprintf(temp, "%c", option);
      strcat(optchar, temp);
      break;
    case 'r':
      gtk_rc_parse("rmt.rc");
      sprintf(temp, "%c", option);
      strcat(optchar, temp);
      break;
    default:
      gtk_rc_parse("web100.rc"); 
      break;
  } 

  ag=web100_attach("localhost");
  conn=web100_find_connection(ag, cid);

  data_init();

  gethostname(hostname, (size_t) 60);

  topwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_ensure_style(topwin); 
  if((rcstyle = gtk_rc_get_style(topwin))==NULL)
    rcstyle = gtk_widget_get_default_style();
  font = rcstyle->font;
#ifdef RECEIVE
  strcpy(titlebar, "Receive tuner@");
#else
  strcpy(titlebar, "Send tuner@");
#endif
  strcat(titlebar, hostname);
  gtk_window_set_title (GTK_WINDOW (topwin), titlebar);
  define_topwin();
  gtk_widget_show(topwin);

  gtk_main();
}

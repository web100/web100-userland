
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
#include <math.h> 
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include "web100.h"
#include "wcpie.h" 
#include "web100gui.h"

#define SND 0 
#define CNG 1
#define RCV 2

struct web100_agent *agent;
struct web100_connection *conn;
struct web100_snap *snap;
struct web100_var *wv[3];
u_int32_t val[3], delta[3]; 
float new_ewma[3], old_ewma[3], total;

GtkWidget *pie, *clist; 
GtkStyle *rcstyle;
GdkFont *font;

char valtext[40];
char hostname[60];
char ftuple[60];
gint hsize, vsize;
int update_id;
float update_interval = 1.0;

gint update_pie(gpointer);
void markup_pie(void); 

float ewma(float new_val, float old_ewma, float weight)
{
  float new_ewma;

  new_ewma = weight*new_val + (1-weight)*old_ewma;
  return(new_ewma);
}

#define CHECKMAX(s) {if (val <= (m=(s)*decade)) { return(m); }}

float newmax(float val, float oldmax)
{
	float m, decade;

	if (val > oldmax){
		decade = pow(10, floor(log10(val))); 
		CHECKMAX(1.0); 
		CHECKMAX(2.0);
		CHECKMAX(5.0);
		CHECKMAX(10.0); // should be redundant
	}
	else return oldmax;
}

void new_pie(char *cid)
{ 
  int ii;

  agent = web100_attach("localhost");
  conn = web100_find_connection(agent, cid); 

  wv[SND] = web100_find_var(conn, "SndLimTimeSender");
  wv[CNG] = web100_find_var(conn, "SndLimTimeCwnd");
  wv[RCV] = web100_find_var(conn, "SndLimTimeRwin");

  snap = web100_get_snapshot(conn, web100_def_file); 

  sprintftuple(ftuple, conn);

  markup_pie(); 

  update_id = gtk_timeout_add(update_interval * 1000,
      (GtkFunction) update_pie, NULL); 
  update_pie(NULL);
}

gint update_pie(gpointer data)
{ 
float per_snd, per_rcv, per_cng;
int ii;

  free(priorsnap);
  snap = web100_get_snapshot(conn, web100_def_file); 
  for(ii=0;ii<3;ii++) delta[ii] = web100_delta_any32(wv[ii], lastsnap,
                                                     priorsnap);
  for(ii=0;ii<3;ii++){ 
    old_ewma[ii] = new_ewma[ii]; 
    new_ewma[ii] = ewma(delta[ii], old_ewma[ii], 0.4); 
  }
  total = new_ewma[0] + new_ewma[1] + new_ewma[2]; 

  if(total){
    per_snd = new_ewma[0]/total;
    per_cng = new_ewma[1]/total;
    per_rcv = new_ewma[2]/total;

    gtk_adjustment_set_value(WC_PIE(pie)->adjustment[0],
                             new_ewma[0]/total);
    gtk_adjustment_set_value(WC_PIE(pie)->adjustment[1],
                             new_ewma[1]/total);
    gtk_adjustment_set_value(WC_PIE(pie)->adjustment[2],
                             new_ewma[2]/total);

    sprintf(valtext, "%.3f", per_snd);
    gtk_clist_set_text(GTK_CLIST(clist), 0, 1, valtext);
    sprintf(valtext, "%.3f", per_rcv);
    gtk_clist_set_text(GTK_CLIST(clist), 1, 1, valtext);
    sprintf(valtext, "%.3f", per_cng);
    gtk_clist_set_text(GTK_CLIST(clist), 2, 1, valtext);
  }

  gtk_widget_show(pie);
  return TRUE;
}

void new_interval(GtkAdjustment *adj, gpointer data)
{ 
  gtk_timeout_remove(update_id);
  update_interval = adj->value; 
  update_id = gtk_timeout_add(update_interval * 1000,
                              (GtkFunction) update_pie, NULL);
}

void markup_pie()
{ 
  GtkWidget *window, *vbox, *hbox, *frame, *entry, *hboxb, *vboxb, *align,
            *updatescale, *button;
  GtkAdjustment *adj, *adjarray[3];
  char titlebar[65]; 
  char *itext[2] = {NULL, NULL}; 
  int ii;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_ensure_style(window);
  if((rcstyle = gtk_rc_get_style(window))==NULL)
    rcstyle = gtk_widget_get_default_style();
  font = rcstyle->font;
  strcpy(titlebar, "Triage Pie Chart@");
  strcat(titlebar, hostname);
  gtk_window_set_title (GTK_WINDOW (window), titlebar); 
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit), NULL); 
  gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, FALSE);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_widget_show(vbox);

  hbox = gtk_hbox_new(FALSE, 0); 
  hboxb = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), hboxb, TRUE, FALSE, 0);
  gtk_widget_show(hboxb);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_label(GTK_FRAME(frame), "TCP session name");
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.0); 
  gtk_box_pack_start(GTK_BOX(hboxb), frame, FALSE, FALSE, 0);
  gtk_widget_show(frame);

  entry = gtk_entry_new(); 
  gtk_entry_set_editable(GTK_ENTRY(entry), FALSE);

  hsize = gdk_string_width(font, ftuple);
  hsize += gdk_string_width(font, "w");
  vsize = gdk_string_height(font, ftuple);
  gtk_widget_set_usize(entry, hsize, 2*vsize);
  gtk_entry_set_text(GTK_ENTRY(entry), ftuple);
  gtk_container_add(GTK_CONTAINER(frame), entry);
  gtk_widget_show(entry);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_label(GTK_FRAME(frame), "ID");
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.0); 
  gtk_box_pack_start(GTK_BOX(hboxb), frame, FALSE, FALSE, 0);
  gtk_widget_show(frame);

  entry = gtk_entry_new();

  gtk_entry_set_editable(GTK_ENTRY(entry), FALSE);
  hsize = gdk_string_width(font, conn->cid);
  hsize += gdk_string_width(font, "w");
  gtk_widget_set_usize(entry, hsize, 1.6*vsize);
  gtk_entry_set_text(GTK_ENTRY(entry), conn->cid); 
  gtk_container_add(GTK_CONTAINER(frame), entry);
  gtk_widget_show(entry);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
  gtk_widget_show(hbox);

  hbox = gtk_hbox_new(FALSE, 0);
  vboxb = gtk_vbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), vboxb, TRUE, TRUE, 10);
  gtk_widget_show(vboxb);

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.1, 10, 0.1, 1, 2)); 
  updatescale=gtk_hscale_new((GtkAdjustment *) adj); 
  gtk_signal_connect(GTK_OBJECT(adj),"value_changed",
      GTK_SIGNAL_FUNC(new_interval), NULL);
  gtk_box_pack_start(GTK_BOX(vboxb), updatescale, FALSE, FALSE, 0);
  gtk_widget_show(updatescale);

  align = gtk_alignment_new(0.5,0.5,1.0,0.0);
  gtk_box_pack_start(GTK_BOX(hbox), align, TRUE, FALSE, 10); 
  gtk_widget_show(align);

  clist = gtk_clist_new(2); 
  gtk_clist_set_column_width(GTK_CLIST(clist), 0, 40);
  gtk_clist_set_column_width(GTK_CLIST(clist), 1, 40);
  gtk_clist_set_column_justification(GTK_CLIST(clist), 0, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_column_justification(GTK_CLIST(clist), 1, GTK_JUSTIFY_RIGHT);

  for(ii=0;ii<3;ii++)
    gtk_clist_append(GTK_CLIST(clist), itext);

  gtk_clist_set_text(GTK_CLIST(clist), 0, 0, "%Snd");
  gtk_clist_set_text(GTK_CLIST(clist), 1, 0, "%Rcv");
  gtk_clist_set_text(GTK_CLIST(clist), 2, 0, "%Cng");

  gtk_container_add(GTK_CONTAINER(align), clist);
  gtk_widget_show(clist);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
  gtk_widget_show(hbox); 

  align = gtk_alignment_new(0.5,0.5,0.0,0.0);

  frame = gtk_frame_new(NULL);   
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN); 
  gtk_container_add(GTK_CONTAINER(align), frame); 
  gtk_widget_show(frame); 

  adjarray[0] = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 1.0, 0.01, 0.1, 0));
  adjarray[1] = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 1.0, 0.01, 0.1, 0));
  adjarray[2] = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 1.0, 0.01, 0.1, 0)); 
  pie = wc_pie_new(adjarray);
  wc_pie_set_update_policy (WC_PIE(pie), GTK_UPDATE_CONTINUOUS);
  gtk_container_add(GTK_CONTAINER(frame), pie);
  gtk_widget_show(pie); 
  gtk_box_pack_start(GTK_BOX(vbox), align,
      FALSE, FALSE, 10);
  gtk_widget_show(align);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
  gtk_widget_show(hbox);

  button = gtk_button_new_with_label(" Close ");
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 3);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(gtk_main_quit), NULL); 

  gtk_widget_show(window);
}


int main(int argc, char *argv[])
{ 
  int option; 

  gtk_init(&argc, &argv);

  if (argc < 2) {
    printf("Usage: triage ConnectionID [-l(ocal)|-r(emote)]\n");
    exit(2);
  }
  option = getopt(argc, argv, "lr");

  switch(option)
  {
    case 'l':
      gtk_rc_parse("lcl.rc");
      break;
    case 'r':
      gtk_rc_parse("rmt.rc");
      break;
    default:
      gtk_rc_parse("web100.rc");
      break;
  } 
 
  gethostname(hostname, (size_t) 60); 

  new_pie(argv[1]);

  gtk_main();
}








#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 2402
#define SERVER_IP "127.0.0.1"
#define MAX_LINE 1024

char buf[MAX_LINE];

int pic_array[400][400][3];
char *EXIT_STRING = "exit";
char *START_STRING = "OSSP 그림채팅\n\0";
char *MAIN_MENU = "\n\t서비스 번호를 선택하세요.\n 1.방 개설 \n 2.방 참여\n\n\n\n<번호/명령(go,exit)>\n\0 ";
char *CREAT_ROOM = "enter chatting room subject ";

fd_set read_fds;
int first_time = 0;
int menu_num = 0;
int  s;
int whats_number = 0;
int maxfdp1;
int timer=0;

//큐를 위한 배열 적당히 크게 잡아둔다.
char           Queue[999];
//현재 큐 안에서의 시작점(위치).
unsigned short QueuePosition = 0;

static void chogiwha(GtkWidget *widget, int whts);
/* Backing pixmap for drawing area */
static GdkPixmap *pixmap = NULL;

GtkWidget *colorseldlg = NULL;
GtkWidget *color_select_area = NULL;

GdkColor user_color; // color
GdkColor rec_color;
int user_brush_size = 10;
static void color_changed_cb(GtkWidget *widget,
   GtkColorSelection *colorsel)
{
   GdkColor ncolor;

   gtk_color_selection_get_current_color(colorsel, &ncolor);

   gtk_widget_modify_bg(color_select_area, GTK_STATE_NORMAL, &ncolor);
}

static gboolean area_event(GtkWidget *widget,
   GdkEvent *event,
   gpointer client_data)
{
   gint handled = FALSE;
   gint response;
   GtkColorSelection *colorsel;

   if (event->type == GDK_BUTTON_PRESS)
   {
      handled = TRUE;

      if (colorseldlg == NULL)
      {
         colorseldlg = gtk_color_selection_dialog_new("Select background color");

      }
      colorsel = GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(colorseldlg)->colorsel);

      gtk_color_selection_set_previous_color(colorsel, &user_color);
      gtk_color_selection_set_current_color(colorsel, &user_color);
      gtk_color_selection_set_has_palette(colorsel, TRUE);

      g_signal_connect(colorsel, "color_changed",
         G_CALLBACK(color_changed_cb), (gpointer)colorsel);
      response = gtk_dialog_run(GTK_DIALOG(colorseldlg));

      if (response == GTK_RESPONSE_OK)
      {
         gtk_color_selection_get_current_color(colorsel, &user_color);

      }
      else
      {
         gtk_widget_modify_bg(color_select_area, GTK_STATE_NORMAL, &user_color);

      }

      gtk_widget_hide(colorseldlg);
   }
   return handled;
}

static gboolean destroy_window(GtkWidget *widget, GdkEvent *event, gpointer client_data)
{
   gtk_main_quit();
   return TRUE;
}
/* Create a new backing pixmap of the appropriate size */
static gint configure_event(GtkWidget         *widget,
   GdkEventConfigure *event)
{
   if (pixmap)
      gdk_pixmap_unref(pixmap);

   pixmap = gdk_pixmap_new(widget->window,
      widget->allocation.width,
      widget->allocation.height,
      -1);
   gdk_draw_rectangle(pixmap,
      widget->style->white_gc,
      TRUE,
      0, 0,
      widget->allocation.width,
      widget->allocation.height);

   if (first_time == 0)
   {
      first_time++;
      chogiwha(widget, whats_number);
      printf("탈출");
   }
   return TRUE;
}
/* Redraw the screen from the backing pixmap */
static gint expose_event(GtkWidget      *widget,
   GdkEventExpose *event)
{
   gdk_draw_pixmap(widget->window,
      widget->style->bg_gc[GTK_WIDGET_STATE(widget)],
      pixmap,
      event->area.x, event->area.y,
      event->area.x, event->area.y,
      event->area.width, event->area.height);
   return FALSE;
}
/* Draw a rectangle on the screen */
int coor_x, coor_y;
char bufall_recv[40];
static void chogiwha(GtkWidget *widget, int whts)
{
   char test[29];
   char * ptr;
   int col_pix1;
   GdkRectangle user_update_rect;
   if (whats_number != 0)
   {
      recv(s, test, 29, 0);
      while (coor_x != 399 || coor_y != 399)
      {
         recv(s, test, 29, 0);
         ptr = strtok(test, " ");
         coor_x = atoi(ptr);
         ptr = strtok(NULL, " ");
         coor_y = atoi(ptr);
         ptr = strtok(NULL, " ");
         rec_color.red = atoi(ptr);
         ptr = strtok(NULL, " ");
         rec_color.green = atoi(ptr);
         ptr = strtok(NULL, " ");
         rec_color.blue = atoi(ptr);
         ptr = strtok(NULL, " ");
         col_pix1 = atoi(ptr);

         user_update_rect.x = coor_x;
         user_update_rect.y = coor_y;
         user_update_rect.width = 1;
         user_update_rect.height = 1;
         if (rec_color.red != 0 || rec_color.green != 0 || rec_color.blue != 0)
         {
            gdk_gc_set_rgb_fg_color(widget->style->bg_gc[0], &rec_color);
            gdk_draw_rectangle(pixmap,
               widget->style->bg_gc[0],
               TRUE,
               user_update_rect.x, user_update_rect.y,
               user_update_rect.width, user_update_rect.height);
            gtk_widget_draw(widget, &user_update_rect);
         }
      }
   }
}
static void draw_brush(GtkWidget *widget, gdouble x, gdouble y)
{
   GdkRectangle update_rect;
   GdkRectangle user_update_rect;

   char string_x[4];
   char string_y[4];
   char col_red[6];
   char col_green[6];
   char col_blue[6];
   char col_pix[3];
   char bufall[40];
   char buf_OQ[29];
   int col_pix1;

   sprintf(string_x, "%03d", (int)x);
   sprintf(string_y, "%03d", (int)y);
   sprintf(col_red, "%05d", user_color.red);
   sprintf(col_green, "%05d", user_color.green);
   sprintf(col_blue, "%05d", user_color.blue);
   sprintf(col_pix, "%02d", user_brush_size);

   sprintf(bufall, "%s %s %s %s %s %s\0", string_x, string_y, col_red, col_green, col_blue, col_pix);
   printf("[send]%s\n", bufall);
   send(s, bufall, 40, 0);

   char *ptr;
   int nbyte;
   unsigned short Size = 29;

    //데이터읽음 실제 몇 바이트를 읽었는지 nbyte에
      //저장
   for (int i = 0; i < 100; i++)
   {
      nbyte = recv(s, bufall_recv, 29, MSG_DONTWAIT);
      //읽은것이 있을경우
      if (nbyte > 0)
      {
         strncpy(&Queue[QueuePosition], bufall_recv, nbyte);
         QueuePosition += nbyte;
         while (1)
         {
            //목표한 패킷크기만큼 읽었다면
            if (QueuePosition >= Size)
            {
               strncpy(buf_OQ, Queue, Size);
               QueuePosition -= Size;
               strncpy(Queue, &Queue[Size], QueuePosition);

               ptr = strtok(buf_OQ, " ");
               coor_x = atoi(ptr);
               ptr = strtok(NULL, " ");
               coor_y = atoi(ptr);
               ptr = strtok(NULL, " ");
               rec_color.red = atoi(ptr);
               ptr = strtok(NULL, " ");
               rec_color.green = atoi(ptr);
               ptr = strtok(NULL, " ");
               rec_color.blue = atoi(ptr);
               ptr = strtok(NULL, " ");
               col_pix1 = atoi(ptr);

               if (coor_x == 999) continue;


               gdk_gc_set_rgb_fg_color(widget->style->bg_gc[0], &rec_color);

               user_update_rect.x = coor_x - 5;
               user_update_rect.y = coor_y - 5;
               user_update_rect.width = col_pix1;
               user_update_rect.height = col_pix1;

               gdk_draw_rectangle(pixmap,
                  widget->style->bg_gc[0],
                  TRUE,
                  user_update_rect.x, user_update_rect.y,
                  user_update_rect.width, user_update_rect.height);
               gtk_widget_draw(widget, &user_update_rect);
            }
            else break;
         }
      }

   }
   gdk_gc_set_rgb_fg_color(widget->style->fg_gc[0], &user_color);

   update_rect.x = (int)x - 5;
   update_rect.y = (int)y - 5;
   update_rect.height = user_brush_size;
   update_rect.width = user_brush_size;

   gdk_draw_rectangle(pixmap,
      widget->style->fg_gc[0],
      TRUE,
      update_rect.x, update_rect.y,
      update_rect.width, update_rect.height);
   gtk_widget_draw(widget, &update_rect);
}
static gint button_press_event(GtkWidget *widget, GdkEventButton *event)
{

   if (event->button == 1 && pixmap != NULL)
   {

      if ((int)(event->x - 0) < 0) event->x = 0;
      if ((int)(event->x - 0) >= 400) event->x = 400;
      if ((int)(event->y - 0) < 0) event->y = 0;
      if ((int)(event->y - 0) >= 400) event->y = 400;
      //0의 영역
      if (whats_number == 0)
      {
        	 if ((event->x - 194) > 0) event->x=194;
        	if( (event->y - 194) > 0) event->y=194;
        	draw_brush(widget, event->x, event->y);
      }
      //1의영역
      else if (whats_number == 1)
      {
    	  if ((event->x - 205) < 0) event->x=205;
    	  if( (event->y - 194) > 0) event->y=194;
    	         	draw_brush(widget, event->x, event->y);

      }
      //2의 영역
      else if (whats_number == 2)
            {
    	  if ((event->x - 194) > 0) event->x=194;
    	         	if( (event->y - 205) < 0) event->y=205;
    	         	draw_brush(widget, event->x, event->y);


            }
            //3의 영역
            else if (whats_number == 3)
            {
            	 if ((event->x - 205) < 0) event->x=205;
            	  if( (event->y - 205) < 0) event->y=205;
            	   	draw_brush(widget, event->x, event->y);
            }
   }
   return TRUE;
}

static gint motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
   int x, y;
   GdkModifierType state;
   if (event->is_hint)
      gdk_window_get_pointer(event->window, &x, &y, &state);
   else
   {
      x = event->x;
      y = event->y;
      state = event->state;
   }
   if (state & GDK_BUTTON1_MASK && pixmap != NULL)
   {
      if ((int)(event->x - 0) < 0) event->x = 0;
      if ((int)(event->x - 0) >= 400) event->x = 399;
      if ((int)(event->y - 0) < 0) event->y = 0;
      if ((int)(event->y - 0) >= 400) event->y =399;
      if (whats_number == 0)
          {
            	 if ((event->x - 194) > 0) event->x=194;
            	if( (event->y - 194) > 0) event->y=194;
            	draw_brush(widget, event->x, event->y);
          }
          //1의영역
          else if (whats_number == 1)
          {
        	  if ((event->x - 205) < 0) event->x=205;
        	  if( (event->y - 194) > 0) event->y=194;
        	         	draw_brush(widget, event->x, event->y);
          }
          //2의 영역
          else if (whats_number == 2)
                {
        	  if ((event->x - 194) > 0) event->x=194;
        	        if( (event->y - 205) < 0) event->y=205;
        	        draw_brush(widget, event->x, event->y);
                }
                //3의 영역
                else if (whats_number == 3)
                {
                	 if ((event->x - 205) < 0) event->x=205;
                	  if( (event->y - 205) < 0) event->y=205;
                	   	draw_brush(widget, event->x, event->y);
                }
   	   	   }
   return TRUE;
}

void quit()
{
   send(s, "exit", 5, 0);
   gtk_exit(0);
}
int tcp_connect(int af, char *servip, unsigned short port);
void errquit(char *mesg) { perror(mesg); exit(1); }
int k = 0;
int main()
{

   int select_num;
   int len = strlen(START_STRING);
   int len2 = strlen(MAIN_MENU);

   char *ptr;
   char bufall[MAX_LINE];
   char first_buf[40];
   char bufmsg[MAX_LINE];
   char recvmsg[10240];

   //AF_INET IPv4프르토콜 사용
   s = tcp_connect(AF_INET, SERVER_IP, SERVER_PORT);
   if (s == -1)
      errquit("tcp_connect fail");

   maxfdp1 = s + 1;
   FD_ZERO(&read_fds);

   recvmsg[0] = '\0';
   while (strstr(recvmsg, "X") == NULL)
   {
      FD_SET(0, &read_fds);
      FD_SET(s, &read_fds);

      if (select(maxfdp1, &read_fds, NULL, NULL, NULL) < 0)
         errquit("select fail");

      if (FD_ISSET(s, &read_fds))
      {
         int nbyte = recv(s, recvmsg, 10240, 0);
         if (strstr(recvmsg, "OSSP") != NULL)
            recvmsg[len + len2] = '\0';
         //X가 포함 되어 있으면 바로 whlie문을 빠져나감.
         if (strstr(recvmsg, "X"))
         {
            break;
         }
         if (nbyte>0)
         {
            printf("%s\n", recvmsg);
         }
      }
      if (FD_ISSET(0, &read_fds))
      {
         if (fgets(bufmsg, 40, stdin))
         {
            if (send(s, bufmsg, 40, 0)<0)
               puts("Error : write error on socket.");
            if (strstr(bufmsg, EXIT_STRING) != NULL)
            {
               puts("Good bye.");
               close(s);
               exit(0);
            }
         }
      }
   }
   recvmsg[4] = '\0';
   ptr = strtok(recvmsg, " ");
   ptr = strtok(NULL, " ");
   whats_number = atoi(ptr);
   printf("<whats_number>%d\n", whats_number);
   recvmsg[0] = '\0';

   GtkWidget *window1;
   GtkWidget *window2;
   GtkWidget *drawing_area;
   GtkWidget *vbox;
   GtkWidget *button;

   gtk_init(NULL, NULL);
   window1 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   window2 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_widget_set_name(window1, "Test Input");

   gtk_window_set_title(GTK_WINDOW(window2), "Color Selection");
   gtk_window_set_policy(GTK_WINDOW(window2), TRUE, TRUE, TRUE);

   g_signal_connect(window2, "delete-event", G_CALLBACK(destroy_window), (gpointer)window2);

   color_select_area = gtk_drawing_area_new();

   user_color.red = 0;
   user_color.blue = 65535;
   user_color.green = 0;

   gtk_widget_modify_bg(color_select_area, GTK_STATE_NORMAL, &user_color);
   gtk_widget_set_size_request(GTK_WIDGET(color_select_area), 200, 200);
   gtk_widget_set_events(color_select_area, GDK_BUTTON_PRESS_MASK);

   g_signal_connect(GTK_OBJECT(color_select_area), "event", GTK_SIGNAL_FUNC(area_event), (gpointer)color_select_area);

   gtk_container_add(GTK_CONTAINER(window2), color_select_area);


   vbox = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER(window1), vbox);
   gtk_widget_show(vbox);

   gtk_signal_connect(GTK_OBJECT(window1), "destroy",
      GTK_SIGNAL_FUNC(quit), NULL);

   /* Create the drawing area */

   drawing_area = gtk_drawing_area_new();
   gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), 400, 400);
   gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);

   gtk_widget_show(drawing_area);

   /* Signals used to handle backing pixmap */

   gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event",
      (GtkSignalFunc)expose_event, NULL);
   gtk_signal_connect(GTK_OBJECT(drawing_area), "configure_event",
      (GtkSignalFunc)configure_event, NULL);

   /* Event signals */

   gtk_signal_connect(GTK_OBJECT(drawing_area), "motion_notify_event",
      (GtkSignalFunc)motion_notify_event, NULL);
   gtk_signal_connect(GTK_OBJECT(drawing_area), "button_press_event",
      (GtkSignalFunc)button_press_event, NULL);

   gtk_widget_set_events(drawing_area, GDK_EXPOSURE_MASK
      | GDK_LEAVE_NOTIFY_MASK
      | GDK_BUTTON_PRESS_MASK
      | GDK_POINTER_MOTION_MASK
      | GDK_POINTER_MOTION_HINT_MASK);

   /* .. And a quit button */
   button = gtk_button_new_with_label("Quit");
   gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

   gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(gtk_widget_destroy),
      GTK_OBJECT(window1));

   gtk_widget_show(button);
   gtk_widget_show(window1);
   gtk_widget_show(color_select_area);
   gtk_widget_show(window2);

   gtk_main();
   return 0;
}
int tcp_connect(int af, char *servip, unsigned short port)
{
   struct sockaddr_in servaddr;
   struct timeval tout;

   //AF_INET,SOCK_STREAM,특정 프로토콜 지정
   //IPv4지정.TCP지정.
   if ((s = socket(af, SOCK_STREAM, 0)) < 0)
      return -1;
   bzero((char *)&servaddr, sizeof(servaddr));
   servaddr.sin_family = af;
   inet_pton(AF_INET, servip, &servaddr.sin_addr);
   servaddr.sin_port = htons(port);
   if (connect(s, (struct sockaddr *)&servaddr, sizeof(servaddr))< 0)
      return -1;

   tout.tv_sec = 0;
   tout.tv_usec = 1000;
   setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof(tout));
   return s;
}

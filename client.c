#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT 3334
#define BUF_SIZE 1024

typedef struct {
    GtkWidget *entry_name;   // Name input field
    GtkWidget *entry_message; // Chat message input field
    GtkWidget *textview;     // Chat display TextView
    int sock;
} Widgets;

void send_message(GtkButton *button, Widgets *w) {
    const gchar *name = gtk_entry_get_text(GTK_ENTRY(w->entry_name));
    const gchar *message = gtk_entry_get_text(GTK_ENTRY(w->entry_message));
    
    if (strlen(name) == 0 || strlen(message) == 0) {
        return; // Do nothing if name or message is empty
    }

    // Prepare message to send
    char msg[BUF_SIZE];
    snprintf(msg, sizeof(msg), "%s: %s", name, message);

    // Send message to server
    send(w->sock, msg, strlen(msg), 0);

    // Clear the message input field
    gtk_entry_set_text(GTK_ENTRY(w->entry_message), "");
}

// 파일을 전송하는 함수
void send_file(const char *filename, int sock) {
    int file = open(filename, O_RDONLY);
    if (file == -1) {
        perror("파일 열기 실패");
        return;
    }

    // 파일 크기 구하기
    off_t file_size = lseek(file, 0, SEEK_END);
    lseek(file, 0, SEEK_SET);

    // 파일 크기를 먼저 서버에 전송
    send(sock, &file_size, sizeof(file_size), 0);

    // 파일 내용을 읽고 전송
    char buffer[BUF_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(file, buffer, sizeof(buffer))) > 0) {
        send(sock, buffer, bytes_read, 0);
    }

    close(file);
}

// 파일을 선택하는 함수
void select_file(GtkButton *button, Widgets *w) {
    GtkWidget *dialog;
    GtkFileChooser *chooser;
    GtkResponseType result;
    
    // 파일 선택 다이얼로그 생성
    dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Open", GTK_RESPONSE_ACCEPT,
                                         NULL);

    result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_ACCEPT) {
        // 선택한 파일의 경로 가져오기
        chooser = GTK_FILE_CHOOSER(dialog);
        char *filename = gtk_file_chooser_get_filename(chooser);

        // 파일 선택 경로를 텍스트뷰에 표시
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->textview));
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(buffer, &iter);
        gtk_text_buffer_insert(buffer, &iter, "File selected: ", -1);
        gtk_text_buffer_insert(buffer, &iter, filename, -1);
        gtk_text_buffer_insert(buffer, &iter, "\n", -1);

        // 서버에 파일 전송
        send_file(filename, w->sock);

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}


void *receive_messages(void *arg) {
    Widgets *w = (Widgets *)arg;
    char buffer[BUF_SIZE];
    
    while (1) {
        int len = recv(w->sock, buffer, sizeof(buffer), 0);
        if (len <= 0) break;
        
        buffer[len] = '\0';

        GtkTextBuffer *buffer_widget = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->textview));
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(buffer_widget, &iter);
        gtk_text_buffer_insert(buffer_widget, &iter, buffer, -1);
        gtk_text_buffer_insert(buffer_widget, &iter, "\n", -1);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    Widgets *w = g_slice_new(Widgets);

    // Create main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat Application");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create main layout box
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Top layout: name input and connect button
    GtkWidget *hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_top, FALSE, FALSE, 0);

    GtkWidget *label_name = gtk_label_new("Name");
    gtk_box_pack_start(GTK_BOX(hbox_top), label_name, FALSE, FALSE, 0);

    w->entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_name), "Enter your name");
    gtk_box_pack_start(GTK_BOX(hbox_top), w->entry_name, TRUE, TRUE, 0);

    GtkWidget *button_connect = gtk_button_new_with_label("Connect");
    gtk_box_pack_start(GTK_BOX(hbox_top), button_connect, FALSE, FALSE, 0);

    // Chat display (with scroll)
    GtkWidget *scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_win, TRUE, TRUE, 0);

    w->textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(w->textview), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled_win), w->textview);

    // Bottom layout: message input and buttons
    GtkWidget *hbox_bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_bottom, FALSE, FALSE, 0);

    w->entry_message = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_message), "Type your message");
    gtk_box_pack_start(GTK_BOX(hbox_bottom), w->entry_message, TRUE, TRUE, 0);

    GtkWidget *button_send = gtk_button_new_with_label("Send");
    gtk_box_pack_start(GTK_BOX(hbox_bottom), button_send, FALSE, FALSE, 0);

    GtkWidget *button_file = gtk_button_new_with_label("File");
    gtk_box_pack_start(GTK_BOX(hbox_bottom), button_file, FALSE, FALSE, 0);

    // Connect signals to buttons
    g_signal_connect(button_send, "clicked", G_CALLBACK(send_message), w);
    g_signal_connect(button_file, "clicked", G_CALLBACK(select_file), w);

    // Create socket and connect to server
    w->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (w->sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(w->sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Create a thread to receive messages from the server
    pthread_t tid;
    pthread_create(&tid, NULL, receive_messages, w);
    pthread_detach(tid);

    gtk_widget_show_all(window);
    gtk_main();

    close(w->sock);
    g_slice_free(Widgets, w);
    return 0;
}
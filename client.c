#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT 3334
#define BUF_SIZE 1024
#define SAVE_DIR "./received_files/"

typedef struct {
    GtkWidget *entry_name;
    GtkWidget *entry_message;
    GtkWidget *textview;
    int sock;
} Widgets;

void send_message(GtkButton *button, Widgets *w) {
    const gchar *name = gtk_entry_get_text(GTK_ENTRY(w->entry_name));
    const gchar *message = gtk_entry_get_text(GTK_ENTRY(w->entry_message));

    if (strlen(name) == 0 || strlen(message) == 0) return;

    char msg[BUF_SIZE];
    snprintf(msg, sizeof(msg), "%s: %s", name, message);
    send(w->sock, msg, strlen(msg), 0);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->textview));
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert(buffer, &iter, msg, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);

    gtk_entry_set_text(GTK_ENTRY(w->entry_message), "");
}

void send_file(const char *filename, int sock) {
    int file = open(filename, O_RDONLY);
    if (file == -1) {
        perror("파일 열기 실패");
        return;
    }

    char buffer[BUF_SIZE];
    ssize_t bytes_read;

    send(sock, "[FILE_TRANSFER]", 16, 0);

    const char *base_filename = strrchr(filename, '/');
    base_filename = (base_filename) ? base_filename + 1 : filename;
    send(sock, base_filename, strlen(base_filename) + 1, 0);

    while ((bytes_read = read(file, buffer, sizeof(buffer))) > 0) {
        send(sock, buffer, bytes_read, 0);
    }

    close(file);
}

void select_file(GtkButton *button, Widgets *w) {
    GtkWidget *dialog;
    GtkFileChooser *chooser;
    GtkResponseType result;

    dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Open", GTK_RESPONSE_ACCEPT,
                                         NULL);

    result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_ACCEPT) {
        chooser = GTK_FILE_CHOOSER(dialog);
        char *filename = gtk_file_chooser_get_filename(chooser);

        send_file(filename, w->sock);

        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->textview));
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(buffer, &iter);
        gtk_text_buffer_insert(buffer, &iter, "File sent: ", -1);
        gtk_text_buffer_insert(buffer, &iter, filename, -1);
        gtk_text_buffer_insert(buffer, &iter, "\n", -1);

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

void save_file(const char *filename, const char *data, size_t data_len) {
    char filepath[BUF_SIZE];
    snprintf(filepath, sizeof(filepath), "%s%s", SAVE_DIR, filename);

    if (mkdir(SAVE_DIR, 0755) == -1 && errno != EEXIST) {
        perror("디렉터리 생성 실패");
        printf("저장 경로: %s\n", SAVE_DIR);
        return;
    } else {
        printf("디렉터리 확인 또는 생성 성공: %s\n", SAVE_DIR);
    }

    FILE *file = fopen(filepath, "wb");
    if (!file) {
        perror("파일 열기 실패");
        return;
    }

    fwrite(data, 1, data_len, file);
    fclose(file);

    printf("파일 저장 완료: %s\n", filepath);
}

void *receive_messages(void *arg) {
    Widgets *w = (Widgets *)arg;
    char buffer[BUF_SIZE];
    char filename[BUF_SIZE];
    int receiving_file = 0;

    while (1) {
        int len = recv(w->sock, buffer, sizeof(buffer), 0);
        if (len <= 0) break;

        buffer[len] = '\0';

        if (strcmp(buffer, "[FILE_TRANSFER]") == 0) {
            receiving_file = 1;
            continue;
        }

        if (receiving_file) {
            strncpy(filename, buffer, sizeof(filename) - 1);
            filename[sizeof(filename) - 1] = '\0';
            len = recv(w->sock, buffer, sizeof(buffer), 0);
            save_file(filename, buffer, len);
            receiving_file = 0;
            continue;
        }

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

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat Application");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_top, FALSE, FALSE, 0);

    GtkWidget *label_name = gtk_label_new("Name");
    gtk_box_pack_start(GTK_BOX(hbox_top), label_name, FALSE, FALSE, 0);

    w->entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_name), "Enter your name");
    gtk_box_pack_start(GTK_BOX(hbox_top), w->entry_name, TRUE, TRUE, 0);

    GtkWidget *scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_win, TRUE, TRUE, 0);

    w->textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(w->textview), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled_win), w->textview);

    GtkWidget *hbox_bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_bottom, FALSE, FALSE, 0);

    w->entry_message = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_message), "Type your message");
    gtk_box_pack_start(GTK_BOX(hbox_bottom), w->entry_message, TRUE, TRUE, 0);

    GtkWidget *button_send = gtk_button_new_with_label("Send");
    gtk_box_pack_start(GTK_BOX(hbox_bottom), button_send, FALSE, FALSE, 0);

    GtkWidget *button_file = gtk_button_new_with_label("File");
    gtk_box_pack_start(GTK_BOX(hbox_bottom), button_file, FALSE, FALSE, 0);

    g_signal_connect(button_send, "clicked", G_CALLBACK(send_message), w);
    g_signal_connect(button_file, "clicked", G_CALLBACK(select_file), w);

    w->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (w->sock == -1) {
        perror("소켓 생성 실패");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(w->sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("서버 연결 실패");
        exit(EXIT_FAILURE);
    }

    pthread_t tid;
    pthread_create(&tid, NULL, receive_messages, w);
    pthread_detach(tid);

    gtk_widget_show_all(window);
    gtk_main();

    close(w->sock);
    g_slice_free(Widgets, w);
    return 0;
}

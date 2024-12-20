#include <gtk/gtk.h>

typedef struct {
    GtkWidget *entry_name;   // Name input field
    GtkWidget *entry_message; // Chat message input field
    GtkWidget *textview;     // Chat display TextView
} Widgets;

void send_message(GtkButton *button, Widgets *w) {
    const gchar *name = gtk_entry_get_text(GTK_ENTRY(w->entry_name));
    const gchar *message = gtk_entry_get_text(GTK_ENTRY(w->entry_message));
    
    if (strlen(name) == 0 || strlen(message) == 0) {
        return; // Do nothing if name or message is empty
    }

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->textview));
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);

    // Append chat content to TextView
    gtk_text_buffer_insert(buffer, &iter, name, -1);
    gtk_text_buffer_insert(buffer, &iter, ": ", -1);
    gtk_text_buffer_insert(buffer, &iter, message, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);

    // Clear the message input field
    gtk_entry_set_text(GTK_ENTRY(w->entry_message), "");
}

void select_file(GtkButton *button, Widgets *w) {
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new("Select File",
                                         NULL,
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Open", GTK_RESPONSE_ACCEPT,
                                         NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);

        // Display selected file path in the chat
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->textview));
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(buffer, &iter);

        gtk_text_buffer_insert(buffer, &iter, "[File Selected]: ", -1);
        gtk_text_buffer_insert(buffer, &iter, filename, -1);
        gtk_text_buffer_insert(buffer, &iter, "\n", -1);

        g_free(filename);
    }
    gtk_widget_destroy(dialog);
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

    gtk_widget_show_all(window);
    gtk_main();

    g_slice_free(Widgets, w);
    return 0;
}
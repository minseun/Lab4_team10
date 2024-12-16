// gui.c
#include <gtk/gtk.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include "network.h"

#define PORT 8080
#define CHUNK_SIZE 1024

GtkTextBuffer *text_buffer;
GtkEntry *entry;

void on_file_upload_button_clicked(GtkWidget *widget, gpointer data);

void *receive_messages_gui(void *arg) {
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';

        // 일반 메시지 출력
        //if (g_utf8_validate(buffer, -1, NULL)) {
            gdk_threads_add_idle((GSourceFunc)gtk_text_buffer_insert_at_cursor, text_buffer);
            gtk_text_buffer_insert_at_cursor(text_buffer, buffer, -1);
            gtk_text_buffer_insert_at_cursor(text_buffer, "\n", -1);
        //}
    }

    gtk_text_buffer_insert_at_cursor(text_buffer, "Server disconnected.\n", -1);
    close(sock);
    return NULL;
}


void send_message(GtkWidget *widget, gpointer data) {
    const char *message = gtk_entry_get_text(entry);
    if (strlen(message) > 0) {
        send_message_to_server(message);

        char formatted_message[BUFFER_SIZE];
        snprintf(formatted_message, sizeof(formatted_message), "Me > %s\n", message);
        gtk_text_buffer_insert_at_cursor(text_buffer, formatted_message, -1);

        gtk_entry_set_text(entry, "");
    }
}

//파일 업로드 버튼 누르면
void on_file_upload_button_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select a file to upload",
                                                   NULL,
                                                   GTK_FILE_CHOOSER_ACTION_OPEN,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   "_Open", GTK_RESPONSE_ACCEPT,
                                                   NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        upload_file_to_server(file_path);
        g_free(file_path);
    }
    gtk_widget_destroy(dialog);
}



int main(int argc, char *argv[]) {
    pthread_t recv_thread;

    // 서버에 연결
    if (connect_to_server("127.0.0.1", PORT) == -1) {
        return EXIT_FAILURE;
    }

    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat Client");
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), text_view, TRUE, TRUE, 0);
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    gtk_text_buffer_insert_at_cursor(text_buffer, "notice: 먼저 아이디를 입력하고 채팅을 이용해주세요\n", -1);

    entry = GTK_ENTRY(gtk_entry_new());
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(entry), FALSE, FALSE, 0);

    GtkWidget *send_button = gtk_button_new_with_label("Send");
    gtk_box_pack_start(GTK_BOX(vbox), send_button, FALSE, FALSE, 0);
    g_signal_connect(send_button, "clicked", G_CALLBACK(send_message), NULL);
    g_signal_connect(entry, "activate", G_CALLBACK(send_message), NULL);

    // 파일 업로드 버튼 추가
    GtkWidget *upload_button = gtk_button_new_with_label("Upload File");
    gtk_box_pack_start(GTK_BOX(vbox), upload_button, FALSE, FALSE, 0);
    g_signal_connect(upload_button, "clicked", G_CALLBACK(on_file_upload_button_clicked), NULL);//파일 업로드 버튼 누르면

    if (pthread_create(&recv_thread, NULL, receive_messages_gui, NULL) != 0) {
        perror("Thread creation failed");
        return EXIT_FAILURE;
    }

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}

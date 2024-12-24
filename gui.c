// gui.c
#include <gtk/gtk.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>

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

        gdk_threads_add_idle((GSourceFunc)gtk_text_buffer_insert_at_cursor, text_buffer);
        gtk_text_buffer_insert_at_cursor(text_buffer, buffer, -1);
        gtk_text_buffer_insert_at_cursor(text_buffer, "\n", -1);
    }

    gtk_text_buffer_insert_at_cursor(text_buffer, "Server disconnected.\n", -1);
    close(sock);
    return NULL;
}


void send_message(GtkWidget *widget, gpointer data) {
    const char *message = gtk_entry_get_text(entry);

    // 문자열 인코딩
    if (!g_utf8_validate(message, -1, NULL)) {
        // UTF-8이 아닌 경우 변환
        gchar *converted_message = g_convert(message, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL);
        if (converted_message != NULL) {
            message = converted_message;  // 변환된 메시지로 갱신
        } else {
            // 변환이 실패하면 원본 메시지를 사용
            message = "";
        }
    }
    
    if (strlen(message) > 0) {
        send_message_to_server(message);

        char formatted_message[BUFFER_SIZE];
        snprintf(formatted_message, sizeof(formatted_message), "Me > %s\n", message);
        gtk_text_buffer_insert_at_cursor(text_buffer, formatted_message, -1);

        gtk_entry_set_text(entry, "");
    }
}


// 파일 업로드 버튼을 눌렀을 때 실행되는 함수
void on_file_upload_button_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Select a file to upload",
                                                   NULL,
                                                   GTK_FILE_CHOOSER_ACTION_OPEN,
                                                   "_Cancel", GTK_RESPONSE_CANCEL,
                                                   "_Open", GTK_RESPONSE_ACCEPT,
                                                   NULL);

    // 파일 선택 후
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        // 파일 확인 창
        GtkWidget *confirm_dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            "파일을 보내시겠습니까?\n%s", file_path
        );

        // 확인 버튼 누르면 업로드
        gint response = gtk_dialog_run(GTK_DIALOG(confirm_dialog));

        if (response == GTK_RESPONSE_YES) {
            upload_file_to_server(file_path);  // 파일 업로드
            printf("File uploaded: %s\n", file_path);

            const char *file_name = g_path_get_basename(file_path);
            download_file_from_server(file_name);
            printf("File downloaded: %s\n", file_name);
        } else {
            printf("파일 업로드가 취소되었습니다.\n");
        }

        gtk_widget_destroy(confirm_dialog);
        g_free(file_path);
    }

    gtk_widget_destroy(dialog);
}




int main(int argc, char *argv[]) {
    pthread_t recv_thread;

    // 서버 연결
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
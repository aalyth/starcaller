import java.net.Socket;
import java.io.IOException;
import java.net.ServerSocket;

class Main {
        private static final int port = 5778;
        private static final int backlog = 200;

        public static void main(final String[] args) throws IOException {
                try (final ServerSocket socket = new ServerSocket(port, backlog)) {
                        final Socket client = socket.accept();
                        System.out.println(new String(client.getInputStream().readAllBytes()));
                }

                System.out.println("hello there");
        }
}

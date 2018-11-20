#include <sys/wait.h>
#include <unistd.h>
#include <string>
#include <vector>

std::string qx(const std::vector<std::string>& args,
               const bool inc_stderr = false) {
  int stdout_fds[2];
  pipe(stdout_fds);

  int stderr_fds[2];
  if (!inc_stderr) {
    pipe(stderr_fds);
  }

  const pid_t pid = fork();
  if (!pid) {
    close(stdout_fds[0]);
    dup2(stdout_fds[1], 1);
    if (inc_stderr) {
      dup2(stdout_fds[1], 2);
    }

    close(stdout_fds[1]);

    if (!inc_stderr) {
      close(stderr_fds[0]);
      dup2(stderr_fds[1], 2);
      close(stderr_fds[1]);
    }

    std::vector<char*> vc(args.size() + 1, NULL);
    for (size_t i = 0; i < args.size(); ++i) {
      vc[i] = const_cast<char*>(args[i].c_str());
    }

    execvp(vc[0], &vc[0]);
    exit(0);
  }

  close(stdout_fds[1]);

  std::string out;
  const int buf_size = 4096;
  char buffer[buf_size];
  do {
    const ssize_t r = read(stdout_fds[0], buffer, buf_size);
    if (r > 0) {
      out.append(buffer, r);
    }
  } while (errno == EAGAIN || errno == EINTR);

  close(stdout_fds[0]);

  if (!inc_stderr) {
    close(stderr_fds[1]);
    do {
      read(stderr_fds[0], buffer, buf_size);
    } while (errno == EAGAIN || errno == EINTR);

    close(stderr_fds[0]);
  }

  int r, status;
  do {
    r = waitpid(pid, &status, 0);
  } while (r == -1 && errno == EINTR);

  return out;
}


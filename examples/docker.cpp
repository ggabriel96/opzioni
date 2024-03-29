#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "opzioni.hpp"

using strv = std::string_view;
template <typename T>
using vec = std::vector<T>;

/**
 * This is a simple simulation of a subset of the docker command line interface using opzioni.
 * It has no affiliation with the Docker project. I just ran `docker --help` and copied the arguments.
 * Hence, none of the docker CLI is my work, I just copied the help text.
 * Also, note that it is not *identical* to the real CLI.
 */
int main(int argc, char const *argv[]) {
  using namespace opzioni;

  constexpr static auto exec =
      Program("exec")
          .intro("Run a command in a running container")
          .add(Help())
          .add(Flg("detach", "d").help("Detached mode: run command in the background"))
          .add(Opt("detach-keys").help("Override the key sequence for detaching a container"))
          .add(Opt("env", "e").help("Set environment variables").action(Append()))
          .add(Opt("env-file").help("Read in a file of environment variables").action(Append()))
          .add(Flg("interactive", "i").help("Keep STDIN open even if not attached"))
          .add(Flg("privileged").help("Give extended privileges to the command"))
          .add(Flg("tty", "t").help("Allocate a pseudo-TTY"))
          .add(Opt("user", "u").help("Username or UID (format: <name|uid>[:<group|gid>])"))
          .add(Opt("workdir", "w").help("Working directory inside the container"))
          .add(Pos("container").help("Name of the target container"))
          .add(Pos("command").help("The command to run in the container").action(Append().gather()));

  constexpr static auto pull = Program("pull")
                                   .intro("Pull an image or a repository from a registry")
                                   .add(Help())
                                   .add(Pos("name").help("The name of the image or repository to pull"))
                                   .add(Flg("all-tags", "a").help("Download all tagged images in the repository"))
                                   .add(Flg("disable-content-trust").help("Skip image verification"))
                                   .add(Opt("platform").help("Set platform if server is multi-platform capable"))
                                   .add(Flg("quiet", "q").help("Supress verbose output"));

  constexpr auto docker =
      Program("docker")
          .version("20.10.1, build 831ebea")
          .intro("A self-sufficient runtime for containers")
          .details("Run 'docker COMMAND --help' for more information on a command.")
          .add(Help())
          .add(Version())
          .add(Opt("config")
                   .help("Location of client config files (default {default_value})")
                   .action(Assign().otherwise("~/.docker")))
          .add(Opt("context", "c")
                   .help(
                       "Name of the context to use to connect to the daemon (overrides DOCKER_HOST env var and default "
                       "context set with \"docker context use\")"))
          .add(Flg("debug", "D").help("Enable debug mode"))
          .add(Opt("host", "H").help("Daemon socket(s) to connect to").action(Append()))
          .add(Opt("log-level", "l")
                   .help("Set the logging level (\"debug\"|\"info\"|\"warn\"|\"error\"|\"fatal\") (default "
                         "{default_value})")
                   .action(Assign().otherwise("info")))
          .add(Flg("tls").help("Use TLS; implied by --tlsverify"))
          .add(Opt("tlscacert")
                   .help("Trust certs signed only by this CA (default {default_value})")
                   .action(Assign().otherwise("~/.docker/ca.pem")))
          .add(Opt("tlscert")
                   .help("Path to TLS certificate file (default {default_value})")
                   .action(Assign().otherwise("~/.docker/cert.pem")))
          .add(Opt("tlskey")
                   .help("Path to TLS key file (default {default_value})")
                   .action(Assign().otherwise("~/.docker/key.pem")))
          .add(Flg("tlsverify").help("Use TLS and verify the remote"))
          .add(exec)
          .add(pull);

  auto const args = docker(argc, argv);

  fmt::print("docker args:\n");
  fmt::print("- config: {}\n", args.as<strv>("config"));
  fmt::print("- context: {}\n", args.as<strv>("context"));
  fmt::print("- debug: {}\n", args.as<bool>("debug"));
  fmt::print("- host: {}\n", args.as<vec<strv>>("host"));
  fmt::print("- log-level: {}\n", args.as<strv>("log-level"));
  fmt::print("- tls: {}\n", args.as<bool>("tls"));
  fmt::print("- tlscacert: {}\n", args.as<strv>("tlscacert"));
  fmt::print("- tlscert: {}\n", args.as<strv>("tlscert"));
  fmt::print("- tlskey: {}\n", args.as<strv>("tlskey"));
  fmt::print("- tlsverify: {}\n", args.as<bool>("tlsverify"));

  if (auto const exec_args = args.cmd("exec")) {
    fmt::print("\nexec args:\n");
    fmt::print("- container: {}\n", exec_args->as<strv>("container"));
    fmt::print("- command: {}\n", exec_args->as<vec<strv>>("command"));
    fmt::print("- detach: {}\n", exec_args->as<bool>("detach"));
    fmt::print("- detach-keys: {}\n", exec_args->as<strv>("detach-keys"));
    fmt::print("- env: {}\n", exec_args->as<vec<strv>>("env"));
    fmt::print("- env-file: {}\n", exec_args->as<vec<strv>>("env-file"));
    fmt::print("- interactive: {}\n", exec_args->as<bool>("interactive"));
    fmt::print("- privileged: {}\n", exec_args->as<bool>("privileged"));
    fmt::print("- tty: {}\n", exec_args->as<bool>("tty"));
    fmt::print("- user: {}\n", exec_args->as<strv>("user"));
    fmt::print("- workdir: {}\n", exec_args->as<strv>("workdir"));
  } else if (auto const pull_args = args.cmd("pull")) {
    fmt::print("\npull args:\n");
    fmt::print("- name: {}\n", pull_args->as<strv>("name"));
    fmt::print("- all-tags: {}\n", pull_args->as<bool>("all-tags"));
    fmt::print("- disable-content-trust: {}\n", pull_args->as<bool>("disable-content-trust"));
    fmt::print("- platform: {}\n", pull_args->as<strv>("platform"));
    fmt::print("- quiet: {}\n", pull_args->as<bool>("quiet"));
  }
}

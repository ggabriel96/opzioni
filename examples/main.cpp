#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <opzioni.hpp>

using namespace std;

constexpr char nl = '\n';

int main(int argc, char const *argv[])
{
    using opz::ArgSpec;

    cout << "argv:\n";
    for_each(argv, argv + argc, [](char const *arg) { cout << arg << nl; });
    cout << nl;

    opz::ArgParser ap;
    ap.add_arg<string>({
        .name = "--name",
        .help = "Your first name",
        .required = true
    });
    ap.add_arg<int>({
        .name = "-v",
        .help = "Level of verbosity",
        .choices = {0, 1, 2, 3, 4},
        .default_value = 0,
        .implicit_value = 1
    });
    ap.add_arg<vector<int>>({
        .name = "--numbers",
        .help = "A list of numbers"
    });

    auto const args = ap.parse_args(argc, argv);
    cout << "\nNumber of parsed arguments: " << args.size() << nl;
    cout << "name: " << args["name"].as<string>() << nl;
    cout << "v: " << args["v"].as<int>() << nl;
    auto const numbers = args["numbers"].as<vector<int>>();
    cout << "numbers: " << nl;
    for_each(begin(numbers), end(numbers), [](auto const &elem) { cout << "- " << elem << nl; });
}

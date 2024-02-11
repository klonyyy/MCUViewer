import subprocess

# Run git log command to get the hash and format it into a Python file
git_hash = subprocess.check_output(
    [
        "git",
        "log",
        '--pretty=format:#define GIT_INFO_PRESENT%n static constexpr const char* GIT_HASH = "%H";',
        "-n",
        "1",
    ]
).decode()

# Write the output to a Python file, creating it if it doesn't exist
file_path = "./src/gitversion.hpp"
with open(file_path, "w+") as file:
    file.write(git_hash)
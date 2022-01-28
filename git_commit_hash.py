import subprocess

Import("env")

def get_firmware_specifier_build_flag():
    b_commit = subprocess.run(["git", "rev-parse" ,"--short=12", "HEAD"], stdout=subprocess.PIPE, text=True)
    b_branch = subprocess.run(["git", "symbolic-ref", "-q", "--short", "HEAD"], stdout=subprocess.PIPE, text=True)

    build_commit = b_commit.stdout.strip()
    build_branch = b_branch.stdout.strip()
    build_version = build_branch + "+sha." + build_commit
    build_flag = "-D AUTO_VERSION=\\\"" + build_version + "\\\""
    print ("Firmware commit sha: " + build_commit)
    print ("Firmware branch: " + build_branch)
    return (build_flag)

env.Append(
    BUILD_FLAGS=[get_firmware_specifier_build_flag()]
)

#!/usr/bin/env python3
"""
Use Compiler Explorer (https://compiler-explorer.com) to test venus with compilers that are not installed locally,
and to create the Compiler Explorer links of the examples in the documentation.

  python3 tools/compiler_explorer.py test             compile and run the examples with old and new compilers
  python3 tools/compiler_explorer.py test g492 g510   ... with specific compilers (Compiler Explorer ids)
  python3 tools/compiler_explorer.py links            create shortlinks for the examples (prints markdown)

The headers and the example are sent as a multi-file project, nothing is needed on the Compiler Explorer side.
Only the Python standard library is used.
"""

import argparse
import concurrent.futures
import json
import pathlib
import re
import sys
import urllib.error
import urllib.request

API = "https://compiler-explorer.com/api"
ROOT = pathlib.Path(__file__).resolve().parent.parent
INCLUDE_DIR = ROOT / "src" / "executor" / "include"
EXAMPLES = ["executor_example", "pool_executor_example", "dispatcher_example"]
OPTIONS = "-std=c++14 -O2 -pthread -I. -Wall -Wextra -pedantic"

# GCC 4.9.2 is the oldest supported compiler, the others check that nothing breaks in between
# (not gcc 4.9.4: its installation on Compiler Explorer cannot compile #include <memory>)
TEST_COMPILERS = [
    "g490", "g492", "g493", "g540", "g65", "g75", "g85", "g95", "g105", "g114", "g142",
    "clang350", "clang391", "clang600", "clang1000", "clang1500",
]

# compilers shown in the documentation links: the oldest supported and a recent one
LINK_COMPILERS = ["g492", "g142"]


def request(path, body=None):
    data = None if body is None else json.dumps(body).encode()
    req = urllib.request.Request(API + path, data=data,
                                 headers={"Content-Type": "application/json", "Accept": "application/json"})
    with urllib.request.urlopen(req, timeout=180) as response:
        return json.load(response)


def header_files():
    """all venus headers, with paths relative to the include directory, for example 'executor/executor.hpp'"""
    return [{"filename": path.relative_to(INCLUDE_DIR).as_posix(), "contents": path.read_text()}
            for path in sorted(INCLUDE_DIR.rglob("*.hpp"))]


def example_source(name):
    return (ROOT / "examples" / f"{name}.cpp").read_text()


def strip_ansi(text):
    return re.sub(r"\x1b\[[0-9;]*m?K?", "", text)


def compile_and_run(compiler, source):
    body = {
        "source": source,
        "files": header_files(),
        "lang": "c++",
        "options": {
            "userArguments": OPTIONS,
            "filters": {"execute": True},
            "compilerOptions": {"executorRequest": True},
        },
    }
    result = request(f"/compiler/{compiler}/compile", body)
    build = result.get("buildResult", {})
    messages = [strip_ansi(line["text"]) for line in build.get("stderr", []) + build.get("stdout", [])]
    output = [line["text"] for line in result.get("stdout", [])]
    errors = [strip_ansi(line["text"]) for line in result.get("stderr", [])]
    return build.get("code"), result.get("code"), messages, output, errors


def available_compilers():
    return {c["id"]: c["name"] for c in request("/compilers/c++?fields=id,name")}


def test(compilers, verbose):
    names = available_compilers()
    unknown = [c for c in compilers if c not in names]
    if unknown:
        print(f"unknown compiler ids: {', '.join(unknown)}")
        return 1

    jobs = [(compiler, example) for compiler in compilers for example in EXAMPLES]
    failures = 0
    # a few parallel requests at most, Compiler Explorer is a shared free service
    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
        futures = {job: pool.submit(compile_and_run, job[0], example_source(job[1])) for job in jobs}
        for compiler, example in jobs:
            try:
                build_code, run_code, messages, output, errors = futures[(compiler, example)].result()
            except urllib.error.URLError as error:
                build_code, run_code, messages, output, errors = None, None, [str(error)], [], []
            warnings = [m for m in messages if "warning:" in m]
            # warnings in venus headers or the examples would show up in the builds of users, they fail the test
            own_warnings = [w for w in warnings if w.startswith(("./executor/", "/app/executor/", "<source>"))]
            ok = build_code == 0 and run_code == 0 and not own_warnings
            failures += not ok
            status = "ok  " if ok else "FAIL"
            note = f", {len(warnings)} warning(s)" if warnings else ""
            print(f"{status} {names[compiler]:<28} {example:<24} build={build_code} exit={run_code}{note}  {' '.join(output)}")
            if not ok or (verbose and warnings):
                for line in messages + errors:
                    print(f"       {line}")

    print(f"\n{len(jobs) - failures}/{len(jobs)} passed")
    return 1 if failures else 0


def client_state(name):
    """a Compiler Explorer multi-file 'tree': the example as main source, the venus headers as closed files"""
    source = example_source(name)
    files = [{"fileId": 1, "isIncluded": True, "isOpen": True, "isMainSource": True,
              "filename": f"{name}.cpp", "content": source, "editorId": 1, "langId": "c++"}]
    for file_id, header in enumerate(header_files(), start=2):
        files.append({"fileId": file_id, "isIncluded": True, "isOpen": False, "isMainSource": False,
                      "filename": header["filename"], "content": header["contents"], "editorId": -1, "langId": "c++"})

    def executor(compiler):
        return {"compiler": {"id": compiler, "options": OPTIONS, "filters": {"execute": True}, "libs": []},
                "compilerVisible": False, "compilerOutputVisible": True, "arguments": "", "argumentsVisible": False,
                "stdin": "", "stdinVisible": False, "wrap": True}

    return {
        "sessions": [{"id": 1, "language": "c++", "source": source, "filename": f"{name}.cpp",
                      "compilers": [], "executors": []}],
        "trees": [{"id": 1, "cmakeArgs": "", "customOutputFilename": "", "isCMakeProject": False,
                   "buildSystem": "none", "compilerLanguageId": "c++", "files": files, "newFileId": len(files) + 1,
                   "compilers": [], "executors": [executor(c) for c in LINK_COMPILERS]}],
    }


def links():
    for name in EXAMPLES:
        for compiler in LINK_COMPILERS:
            build_code, run_code, messages, _, _ = compile_and_run(compiler, example_source(name))
            if build_code != 0 or run_code != 0:
                print(f"{name} does not build or run with {compiler}, not creating a link:\n" + "\n".join(messages))
                return 1
        url = request("/shortener", client_state(name))["url"]
        print(f"- [{name}]({url})")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    test_command = commands.add_parser("test", help="compile and run the examples with old and new compilers")
    test_command.add_argument("compilers", nargs="*", default=TEST_COMPILERS, help="Compiler Explorer compiler ids")
    test_command.add_argument("-v", "--verbose", action="store_true", help="also show warnings of passing builds")
    commands.add_parser("links", help="create Compiler Explorer shortlinks for the examples")
    args = parser.parse_args()

    if args.command == "test":
        return test(args.compilers, args.verbose)
    return links()


if __name__ == "__main__":
    sys.exit(main())

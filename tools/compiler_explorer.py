#!/usr/bin/env python3
"""
Use Compiler Explorer (https://compiler-explorer.com) to test venus with compilers that are not installed locally,
and to create the Compiler Explorer links of the examples in the documentation.

  python3 tools/compiler_explorer.py test             compile and run the examples with old and new compilers
  python3 tools/compiler_explorer.py test g492 g510   ... with specific compilers (Compiler Explorer ids)
  python3 tools/compiler_explorer.py links            create shortlinks for the examples (prints markdown)

On Compiler Explorer, the examples include the single header venus.hpp from GitHub (master) by URL:
the browser downloads it and sends it with the source. These commands do the same, using the local venus.hpp
for 'test' (to test the working tree) and the one on GitHub for 'links' (what the links will use), or the
other one with --github / --local. Only the Python standard library is used.
"""

import argparse
import concurrent.futures
import json
import pathlib
import re
import sys
import urllib.error
import urllib.parse
import urllib.request

API = "https://compiler-explorer.com/api"
ROOT = pathlib.Path(__file__).resolve().parent.parent
SINGLE_HEADER = ROOT / "src" / "executor" / "include" / "venus.hpp"
SINGLE_HEADER_URL = "https://raw.githubusercontent.com/janwilmans/venus/refs/heads/master/src/executor/include/venus.hpp"
EXAMPLES = ["executor_example", "pool_executor_example", "dispatcher_example"]
OPTIONS = "-std=c++14 -O2 -pthread -Wall -Wextra -pedantic"

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


def single_header(from_github):
    if from_github:
        with urllib.request.urlopen(SINGLE_HEADER_URL, timeout=60) as response:
            return response.read().decode()
    return SINGLE_HEADER.read_text()


def online_source(name):
    """the example, with its '#include "executor/..."' lines replaced by one URL include of the single header"""
    lines = []
    replaced = False
    for line in (ROOT / "examples" / f"{name}.cpp").read_text().splitlines():
        if re.match(r'\s*#\s*include\s*"executor/', line):
            if not replaced:
                lines.append(f"#include <{SINGLE_HEADER_URL}>")
                replaced = True
            continue
        lines.append(line)
    return "\n".join(lines) + "\n"


def as_sent_by_browser(source, header):
    """what the Compiler Explorer page sends: URL includes become includes of files named <host><path>"""
    url = urllib.parse.urlparse(SINGLE_HEADER_URL)
    filename = url.netloc + url.path
    source = source.replace(f"#include <{SINGLE_HEADER_URL}>", f'#include "{filename}"')
    return source, [{"filename": filename, "contents": header}]


def strip_ansi(text):
    return re.sub(r"\x1b\[[0-9;]*m?K?", "", text)


def compile_and_run(compiler, source, header):
    source, files = as_sent_by_browser(source, header)
    body = {
        "source": source,
        "files": files,
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


def test(compilers, header, verbose):
    names = available_compilers()
    unknown = [c for c in compilers if c not in names]
    if unknown:
        print(f"unknown compiler ids: {', '.join(unknown)}")
        return 1

    jobs = [(compiler, example) for compiler in compilers for example in EXAMPLES]
    failures = 0
    # a few parallel requests at most, Compiler Explorer is a shared free service
    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
        futures = {job: pool.submit(compile_and_run, job[0], online_source(job[1]), header) for job in jobs}
        for compiler, example in jobs:
            try:
                build_code, run_code, messages, output, errors = futures[(compiler, example)].result()
            except urllib.error.URLError as error:
                build_code, run_code, messages, output, errors = None, None, [str(error)], [], []
            warnings = [m for m in messages if "warning:" in m]
            # warnings in venus or the examples would show up in the builds of users, they fail the test
            own_warnings = [w for w in warnings if "venus.hpp:" in w or w.startswith("<source>")]
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
    """a Compiler Explorer editor with the example, and an executor pane per compiler"""
    def executor(compiler):
        return {"compiler": {"id": compiler, "options": OPTIONS, "filters": {"execute": True}, "libs": []},
                "compilerVisible": False, "compilerOutputVisible": True, "arguments": "", "argumentsVisible": False,
                "stdin": "", "stdinVisible": False, "wrap": True}

    return {
        "sessions": [{"id": 1, "language": "c++", "source": online_source(name), "compilers": [],
                      "executors": [executor(c) for c in LINK_COMPILERS]}],
        "trees": [],
    }


def links(header):
    for name in EXAMPLES:
        for compiler in LINK_COMPILERS:
            build_code, run_code, messages, _, _ = compile_and_run(compiler, online_source(name), header)
            if build_code != 0 or run_code != 0:
                print(f"{name} does not build or run with {compiler}, not creating a link:\n" + "\n".join(messages))
                return 1
    for name in EXAMPLES:
        url = request("/shortener", client_state(name))["url"]
        print(f"- [{name}]({url})")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    test_command = commands.add_parser("test", help="compile and run the examples with old and new compilers")
    test_command.add_argument("compilers", nargs="*", default=TEST_COMPILERS, help="Compiler Explorer compiler ids")
    test_command.add_argument("--github", action="store_true", help="use venus.hpp from GitHub master")
    test_command.add_argument("-v", "--verbose", action="store_true", help="also show warnings of passing builds")
    links_command = commands.add_parser("links", help="create Compiler Explorer shortlinks for the examples")
    links_command.add_argument("--local", action="store_true", help="verify with the local venus.hpp")
    args = parser.parse_args()

    if args.command == "test":
        return test(args.compilers, single_header(args.github), args.verbose)
    return links(single_header(not args.local))


if __name__ == "__main__":
    sys.exit(main())

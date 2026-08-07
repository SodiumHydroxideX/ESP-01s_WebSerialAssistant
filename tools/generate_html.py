Import("env")
import os

def generate_index_html_h(source, target, env):
    src = os.path.join(env["PROJECT_DIR"], "src", "index.html")
    dst = os.path.join(env["PROJECT_DIR"], "src", "index_html.h")

    with open(src, "r", encoding="utf-8") as f:
        html = f.read()

    # 确保 rawliteral 分隔符不会与 HTML 内容冲突
    with open(dst, "w", encoding="utf-8") as f:
        f.write('const char index_html[] PROGMEM = R"rawliteral(\n')
        f.write(html)
        if not html.endswith("\n"):
            f.write("\n")
        f.write(')rawliteral";\n')

    print("generate_html.py: regenerated index_html.h from index.html")

env.AddPreAction("buildprog", generate_index_html_h)

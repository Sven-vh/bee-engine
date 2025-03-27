import os
import sys

def add_license_to_file(file_path, license_text):
    """
    Reads the file's content and writes the license header at the top.
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return

    new_content = content + "\n\n\n" + license_text

    try:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
    except Exception as e:
        print(f"Error writing {file_path}: {e}")

def main(directory, license_file=None):
    if license_file:
        try:
            with open(license_file, 'r', encoding='utf-8') as lf:
                license_text = lf.read()
        except Exception as e:
            print(f"Error reading license file {license_file}: {e}")
            sys.exit(1)
    else:
        # Default license text: Modified MIT License with a contact request.
        license_text = (
            "/*\n"
            "  MIT License\n"
            " \n"
            "  Copyright (c) 2023 Games@Breda University of Applied Sciences\n"
            " \n"
            "  Permission is hereby granted, free of charge, to any person obtaining a copy\n"
            "  of this software and associated documentation files (the \"Software\"), to deal\n"
            "  in the Software without restriction, including without limitation the rights\n"
            "  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
            "  copies of the Software, and to permit persons to whom the Software is\n"
            "  furnished to do so, subject to the following conditions:\n"
            " \n"
            "  The above copyright notice and this permission notice shall be included in all\n"
            "  copies or substantial portions of the Software.\n"
            " \n"
            "  THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
            "  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
            "  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
            "  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
            "  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
            "  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
            "  SOFTWARE.\n"
            " \n"
            "  ---------------------------------------------------------------------------\n"
            " \n"
            "  If you use this software, please drop a message to let us know how and where it's used.\n"
            "  Contact: svanhuessen@gmail.com\n"
            " */\n"
        )

    valid_extensions = ('.h', '.hpp', '.c', '.cpp')

    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(valid_extensions):
                file_path = os.path.join(root, file)
                print(f"Adding license to: {file_path}")
                add_license_to_file(file_path, license_text)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python add_license.py <directory> [license_file]")
        sys.exit(1)

    directory = sys.argv[1]
    license_file = sys.argv[2] if len(sys.argv) >= 3 else None
    main(directory, license_file)

from prompt_toolkit import prompt
from prompt_toolkit import print_formatted_text, HTML

text = prompt("Give me some input: ")
print_formatted_text(HTML('<ansired>This is red</ansired>'))
print_formatted_text(HTML('<ansigreen>This is green</ansigreen>'))
print(f"You said: {text}")

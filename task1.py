import re

def precendence(op):
    return {
        "+" : 1,
        "-" : 1,
        "*" : 2,
        "/" : 2
    }.get(op,0)

def to_postfix(s:str):
    stack = []
    result = []
    s = s.replace('(-', '(0-')
    
    if s.startswith('-'):
        s = '0' + s

    for op in "+-/*()":
        s = s.replace(op, f" {op} ")
    
    tokens = re.findall(r"(\d+\.\d+|\d+|[a-zA-Z_]\w*|[\+\-\*/\(\)])",s)

    for token in tokens:
        if token.replace('.','',1).isdigit() or token[0].isalpha():
            result.append(token)
        elif token == "(":
            stack.append(token)
        elif token == ")":
            while stack and stack[-1] != "(":
                result.append(stack.pop())
            stack.pop()
        else:
            while stack and precendence(stack[-1]) >= precendence(token):
                result.append(stack.pop())
            stack.append(token)
        
    while stack:
        result.append(stack.pop())

    return " ".join(result)

def evaluate(postfix:str, variables:dict):
    stack = []
    ops = {
        "+" : lambda a, b : a + b,
        "-" : lambda a, b : a - b,
        "*" : lambda a, b : a * b,
        "/" : lambda a, b : a / b,
        }
    for token in postfix.split():
        if token.replace('.','',1).isdigit():
            stack.append(float(token))
        elif token[0].isalpha():
            if token not in variables:
                variables[token] = float(input(f"Enter value for {token}: "))
            stack.append(variables[token])
        else:
            right = stack.pop()
            left = stack.pop()
            stack.append(ops[token](left, right))
    return stack[0]

vars = {
    "x":5,
    "y":-8.5,
    "z":2
}

expression = "x+y+z"
postfix = to_postfix(expression)
result = evaluate(postfix, vars)
print(f"Postfix: {postfix}")
print(f"Result: {result}")
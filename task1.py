def precendence(op):
    return {
        "+" : 1,
        "-" : 1,
        "*" : 2,
        "/" : 2
    }.get(op,0)

def infix_to_postfix(s:str):
    stack = []
    result = []
    s = s.replace('(-', '(0-')
    
    if s.startswith('-'):
        s = '0' + s

    for op in "+-/*()":
        s = s.replace(op, f" {op} ")
    
    for token in s.split():
        if token.isdigit() or ( token.startswith('-') and len(token) > 1):
            result.append(token)
        elif token == '(':
            stack.append(token)
        elif token == ")":
            while stack and stack[-1] != '(':
                result.append(stack.pop())
            stack.pop()
        else:
            while stack and precendence(stack[-1]) >= precendence(token):
                result.append(stack.pop())
            stack.append(token)
        
    while stack:
        result.append(stack.pop())

    return " ".join(result)

def evaluate(postfix:str):
    stack = []
    ops = {
        "+" : lambda a, b : a + b,
        "-" : lambda a, b : a - b,
        "*" : lambda a, b : a * b,
        "/" : lambda a, b : a / b,
        }
    for token in postfix.split():
        if token.replace("-",'').isdigit():
            stack.append(int(token))
        else:
            right =  stack.pop()
            left = stack.pop()
            stack.append(ops[token](left, right))
    return stack[0]

expression = "(3+5)*4 - 5 + (5*4-26)/3"
print(evaluate(infix_to_postfix(expression)))
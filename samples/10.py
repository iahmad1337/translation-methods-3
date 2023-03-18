if not 10 == 0:
    print("Success!")

a = 1
b = 0

if not a or b:
    print("Error: invalid precedence for `not`")

a = 1
b = 1
if not a < b:
    print("Error: invalid precedence for `not`! [2]")

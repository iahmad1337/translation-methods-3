if 1:
    print("First nested print\n")
    if 1:
        print("Second nested print\n")
        if 0:
            print("You should never see this message!\n")
    print("this print should have the correct nesting\n")

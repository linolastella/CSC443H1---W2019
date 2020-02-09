with open("names.db", "rb") as f, open("test.db", "wb+") as t_f:
    first_32768 = f.read(3200)
    t_f.write(first_32768)

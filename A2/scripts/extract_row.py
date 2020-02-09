if __name__ == "__main__":
    with open("names.db", "rb") as f:
        row_number = 0x820
        record_size = 64
        print(f.read()[record_size * row_number : record_size + record_size * row_number])

if __name__ == "__main__":
    atexit.register(lambda: stop_perftest(SERVERS + CLIENTS))
    main()
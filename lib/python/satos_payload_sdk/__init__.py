import logging

if __name__ == '__main__':
    logging.basicConfig(    format="%(asctime)s  %(levelname)s %(message)s",
                            datefmt="%Y-%m-%d %H:%M:%S",
                            level=logging.DEBUG
                        )
import logging

if __name__ == '__main__':
    # DEBUG = os.environ.get('DEBUG')
    logging.basicConfig(    format="%(asctime)s  %(levelname)s %(message)s",
                            datefmt="%Y-%m-%d %H:%M:%S"
                        )
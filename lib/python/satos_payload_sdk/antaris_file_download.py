from satos_payload_sdk import antaris_api_common as api_common
from azure.storage.fileshare import ShareFileClient
from azure.storage.fileshare import ShareDirectoryClient
import logging
import time
import os


logger = logging.getLogger("azure.core.pipeline.policies.http_logging_policy")
logger.setLevel(logging.WARNING)

g_FTM="FTM"
g_File_String="File_Conn_Str"
g_Truetwin_Dir="Truetwin_Dir"
g_Share_Name="Share_Name"

class File_Stage():

    def __init__(self, download_file_params,config_data ): 

        self.download_file_params=download_file_params
        self.config_data=config_data
        self.file_name=self.download_file_params.file_path
        self.file_path_remote=self.config_data[g_FTM][g_Truetwin_Dir] + "/" + self.file_name

    def get_file_size(self):
        try:
            size = os.path.getsize("/opt/antaris/outbound/" + f"{self.file_name}")
            return size
        except OSError as e:
            logger.error(f"Failed to get the size of {self.file_name}. Error: {str(e)}")
            return None

    def start_upload(self): 
        file_client = ShareFileClient.from_connection_string(conn_str=self.config_data[g_FTM][g_File_String], share_name=self.config_data[g_FTM][g_Share_Name], file_path=self.file_path_remote)
        try:
            with open("/opt/antaris/outbound/" + f"{self.file_name}", "rb") as source_file:
                file_client.upload_file(source_file)
            return "Success"
        except Exception as e:
            logger.error("Upload failed")
            logger.error(f"Error message: {str(e)}")
            return "Failure"

    def file_check(self):
        parent_dir = ShareDirectoryClient.from_connection_string(conn_str=self.config_data[g_FTM][g_File_String], share_name=self.config_data[g_FTM][g_Share_Name], directory_path=self.config_data[g_FTM][g_Truetwin_Dir])
        file_list = list(parent_dir.list_directories_and_files())
        max_attempts = 3
        attempt = 1
        found=False

        while attempt <= max_attempts:
            for file in file_list:
                time.sleep(1)
                if file["name"] == self.file_name:
                    found = True
                    for _ in range(max_attempts):
                        time.sleep(1)
                        try:
                            if file["size"] == self.get_file_size():
                                print("File uploaded successfully.")
                                logger.info("File uploaded successfully")
                                return "Success"
                        except Exception as e:
                            logger.error(f"Error: {e}")
                            return "Failure"
            if found:
                break
            
            attempt += 1

        if not found:
            logger.error("File not found after maximum attempts.")
            return "Failure"
        
    def file_download(self):
        if api_common.g_TRUETWIN_ENABLE == '1':
            if  self.start_upload() and self.file_check():
                return True
            else:
                return False
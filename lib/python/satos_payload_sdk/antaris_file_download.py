from azure.storage.fileshare import ShareFileClient
import logging

logger = logging.getLogger("azure.core.pipeline.policies.http_logging_policy")
logger.setLevel(logging.WARNING)

g_FTM = "FTM"
g_File_String = "File_Conn_Str"
g_Truetwin_Dir = "Truetwin_Dir"
g_Share_Name = "Share_Name"
g_Outbound_Path_Prefix = "/opt/antaris/outbound/"

class File_Stage():

    def __init__(self, download_file_params,config_data ): 
        self.download_file_params=download_file_params
        self.config_data=config_data
        self.file_name=self.download_file_params.file_path
        self.file_path_remote=self.config_data[g_FTM][g_Truetwin_Dir] + "/" + self.file_name

    def start_upload(self):
        file_path_local = g_Outbound_Path_Prefix + self.download_file_params.file_path
        ret = azure_file_upload(file_path_local, self.config_data[g_FTM][g_File_String], self.config_data[g_FTM][g_Share_Name], self.file_path_remote)

        return ret
        
    def file_download(self):
        if  self.start_upload():
            return True
        else:
            return False
        
def azure_file_upload(file_name, conn_str, share_name, file_path):
    file_client = ShareFileClient.from_connection_string(conn_str, share_name, file_path)
    try:
        with open(file_name, "rb") as source_file:
            file_client.upload_file(source_file)
            return True
    except Exception as e:
        logger.error("Upload  to truetwin failed")
        logger.error(f"Error message: {str(e)}")
        return False
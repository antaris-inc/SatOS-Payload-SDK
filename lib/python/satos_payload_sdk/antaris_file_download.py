import logging

from azure.storage.fileshare import ShareFileClient

from satos_payload_sdk import antaris_api_common as api_common


logger = logging.getLogger("satos_payload_sdk")

azure_logger = logging.getLogger("azure.core.pipeline.policies.http_logging_policy")
azure_logger.setLevel(logging.WARNING)


g_FTM = "FTM"
g_File_String = "File_Conn_Str"
g_Truetwin_Dir = "Truetwin_Dir"
g_Share_Name = "Share_Name"
g_Outbound_Path_Prefix = "/opt/antaris/outbound/"


class File_Stage():

    def __init__(self, download_file_params, config_data):
        self.config_data = config_data
        self.file_path_local = g_Outbound_Path_Prefix + self.download_file_params.file_path
        self.file_path_remote = self.config_data[g_FTM][g_Truetwin_Dir] + "/" + self.file_name

    def file_download(self):
        cl = ShareFileClient.from_connection_string(
                conn_str=self.config_data[g_FTM][g_File_String],
                share_name=self.config_data[g_FTM][g_Share_Name],
                file_path=self.file_path_remote
        )

        try:
            with open(self.file_path_local, "rb") as f:
                cl.upload_file(f)

        except Exception as exc:
            logger.exception("File transfer failed")
            return False

        return True

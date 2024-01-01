#include<Arduino.h>
#include <ESP32_FTPClient.h>

char ftp_server[] = "192.168.1.43";
char ftp_user[]   = "marcel";
char ftp_pass[]   = "TODO Passwd";

// you can pass a FTP timeout and debbug mode on the last 2 arguments
ESP32_FTPClient ftp (ftp_server,ftp_user,ftp_pass, 5000, 2);

void ftp_setup()
{

  ftp.OpenConnection();

  // Get directory content
  ftp.InitFile("Type A");
  String list[128];
  ftp.ChangeWorkDir("/home/marcel/Downloads/from-ftp");

  // Create the new file and send the image
  ftp.ChangeWorkDir("my_new_dir");
  ftp.InitFile("Type I");
  ftp.NewFile("octocat.jpg");
  //ftp.WriteData( octocat_pic, sizeof(octocat_pic) );
  ftp.CloseFile();

  // Create the file new and write a string into it
  ftp.InitFile("Type A");
  ftp.NewFile("hello_world.txt");
  ftp.Write("Hello World");
  ftp.CloseFile();

  ftp.CloseConnection();
}


void upload_file(String filename){
  
}
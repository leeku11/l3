
l3cmd cmd boot : 부트로더진입
l3cmd cmd matrix : 매트릭스출력(키매핑할때)
l3cmd cmd keycode : 매트릭스출력(키매핑완료후)

******주의 config file 은 128바이트로 고정했습니다. 이전은 120바이트 였습니다. 120바이트 쓰려고 하면 에러를 뱉습니다.*************
l3cmd readcfg [filename] : (kbd -> file) config 읽어서 파일에씀
l3cmd writecfg [filename] : (file -> kbd) config 파일에서 읽어서 kbd에씀

******주의 keymap 은 480바이트로 고정했습니다. 이전도 480바이트 였습니다. ************
l3cmd readkey [filename] : (kbd -> file) keymap 읽어서 파일에씀
l3cmd writekey [filename] : (file -> kbd) keymap 파일에서 읽어서 kbd에씀

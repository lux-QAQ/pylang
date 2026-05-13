(base) lux@Lux:~/code/language/python-cpp$ vtune-gui
pure_python_aot_bench
no import, no f-string
SCALE 1
dynamic_classes 1000 9223335890410005232
small_int 2000000 202268616
big_int 120000 64291062482545
functions 1500000 1610275283327897
containers 800000 52390286386
text_exception 300000 12239498
final_checksum 9221786413350975080

pure_python_aot_bench
no import, no f-string
SCALE 1
dynamic_classes 1000 9223335890410005232
small_int 2000000 202268616
big_int 120000 64291062482545
functions 1500000 1610275283327897
containers 800000 52390293128
text_exception 300000 12239498
final_checksum 9221786413350964434

正确答案:
(base) lux@Lux:~/code/language/python-cpp$ python test7.py
pure_python_aot_bench
no import, no f-string
SCALE 1
dynamic_classes 1000 9223335890410005232
small_int 2000000 202268616
big_int 120000 64291062482545
functions 1500000 1610275283327897
containers 800000 52391119150
text_exception 300000 12229166
final_checksum 9221786413349729232

#g++ -std=c++1z -ggdb half_circle.cpp `pkg-config --cflags --libs opencv4`
#g++ -std=c++1z  -ggdb video_cap.cpp `pkg-config --cflags --libs opencv4`

g++ -std=c++1z -o half_circle half_circle.cpp `pkg-config --cflags --libs opencv4`
g++ -std=c++1z -o video_cap video_cap.cpp `pkg-config --cflags --libs opencv4`

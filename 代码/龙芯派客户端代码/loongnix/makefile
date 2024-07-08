TARGET := main
CXX := loongarch64-linux-gnu-g++
LIBS :=  -L/home/heye/software/loongnix/loongnix_qt/lib $(shell pkg-config --libs loongnix_opencv) 
LIBS += -L/home/heye/software/loongnix/gpio_lib -L/home/heye/software/loongnix/loongnix_sqlite3/lib
INCS := -I/home/heye/software/loongnix/loongnix_qt/include  $(shell pkg-config --cflags loongnix_opencv) -I./inc
INCS += -I/home/heye/software/loongnix/gpio_inc
LIBNAME := -lQt5Widgets  -lQt5Gui -lQt5Core -lQt5Network -lQt5Sql -lpthread -lsqlite3  -lgpiodcxx -lgpiod
SOURCES := $(wildcard ./src/*.cpp)
MOC := /home/heye/software/loongnix/loongnix_qt/bin/moc # 设置moc的路径，修改为实际路径

SQLITE3INC := -I/home/heye/software/loongnix/loongnix_sqlite3/include
SQLITE3LIB := -L/home/heye/software/loongnix/loongnix_sqlite3/lib

INCS += $(SQLITE3INC)
LIBS += $(SQLITE3LIB).

# 添加 moc_%.cpp 文件到 SOURCES 变量
SOURCES += $(patsubst ./inc/%.h, ./src/moc_%.cpp, $(wildcard ./inc/*.h))
SOURCES += $(patsubst ./inc/%.hpp, ./src/moc_%.cpp, $(wildcard ./inc/*.hpp))

# 添加 moc_face.cpp 到 SOURCES 变量
SOURCES += $(wildcard ./src/moc_face.cpp)

OBJS:=$(patsubst %.cpp, %.o, $(SOURCES))

# 添加 moc_%.cpp 对应的目标文件到 OBJS 变量
OBJS += $(patsubst ./src/moc_%.cpp, ./src/%.o, $(wildcard ./src/moc_*.cpp))

# moc_%.cpp 规则，用于生成 moc_%.cpp 文件
./src/moc_%.cpp: ./inc/%.h
	$(MOC) $< -o $@

./src/moc_%.cpp: ./inc/%.hpp	
	$(MOC) $< -o $@
	
# 编译目标文件
$(TARGET): $(OBJS)
	$(CXX) $^ -o $@ $(INCS) $(LIBS) $(LIBNAME)
	cp $(TARGET) ./bin
	mv $(TARGET) ~/my_ftp

# 编译源文件为目标文件
%.o: %.cpp
	$(CXX) -c $< -o $@ -std=c++11 $(INCS) $(LIBS) $(LIBNAME)

# 清理
clean:
	rm -f $(TARGET) ./bin/$(TARGET) ~/my_ftp/$(TARGET) $(OBJS)

# 执行清理、编译、运行
execute:
	make clean && make && ./$(TARGET)

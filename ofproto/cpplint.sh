for file in \
       ./xf_*.c \
       ./xf_*.h
do
    astyle $file;
    ./cpplint.py --filter=-whitespace/line_length,-runtime/int,-runtime/sizeof,-runtime/threadsafe_fn,-build/include,-build/header_guard,-readability/casting,-readability/function --counting=detailed $file;
done

#./cpplint.py --filter=-whitespace/line_length,-runtime/threadsafe_fn,-build/include,-build/header_guard,-runtime/sizeof,-readability/casting,-runtime/int,-readability/function --counting=detailed $1

rm -rf *.orig;

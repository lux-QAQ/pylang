# pure_python_aot_bench.py
# 约束：不使用 import，不使用 f-string。
# 说明：本程序内部不计时；请用外部 time/hyperfine/你的 AOT runner 计时。
#      这样可以避免 time/sys/argparse 等标准库开销污染结果。
#
# 调参：SCALE 越大，运行越久。建议先用 SCALE = 1 验证，再逐步增大。

SCALE = 1

DYNAMIC_CLASS_ROUNDS = 1000 * SCALE
SMALL_INT_ROUNDS = 2000000 * SCALE
BIG_INT_ROUNDS = 120000 * SCALE
FUNCTION_ROUNDS = 1500000 * SCALE
CONTAINER_ROUNDS = 800000 * SCALE
TEXT_EXCEPTION_ROUNDS = 300000 * SCALE


class DataDescriptor:
    def __init__(self, delta):
        self.delta = delta

    def __get__(self, obj, typ):
        if obj is None:
            return self
        return obj.__dict__.get("payload", 0) + self.delta + typ.root_bias

    def __set__(self, obj, val):
        obj.__dict__["payload"] = val - self.delta


def dyn_init(self, value=0):
    self.payload = value
    self.more = value + 1


def root_method(self, x):
    return x + self.payload + self.root_bias


def left_method(self, x):
    return self.base(x) + self.left_bias + self.desc


def right_method(self, x):
    return x - self.payload + self.right_bias + self.more


def alt_method(self, x):
    return x + self.root_bias + self.leaf_bias


def make_dynamic_class(i):
    # 这个继承图会迫使 type() 执行 C3 线性化：
    #
    #              R
    #        /  /  |  \  \
    #       A  B   C   D
    #        \ /     \ /
    #        AB       CD
    #           \    /
    #             X
    #
    # X 的 MRO 需要合并 AB/CD 及其父类线性化结果。
    name = str(i)
    root_attrs = {
        "root_bias": i & 255,
        "desc": DataDescriptor(i & 31),
        "__init__": dyn_init,
        "base": root_method,
    }
    R = type("R" + name, (object,), root_attrs)

    A = type("A" + name, (R,), {
        "left_bias": 3 + (i & 7),
        "m": left_method,
        "tag_a": i,
    })
    B = type("B" + name, (R,), {
        "right_bias": 5 + (i & 15),
        "n": right_method,
        "tag_b": i + 1,
    })
    C = type("C" + name, (R,), {
        "left_bias": 7 + (i & 3),
        "m": alt_method,
        "tag_c": i + 2,
    })
    D = type("D" + name, (R,), {
        "right_bias": 11 + (i & 3),
        "n": alt_method,
        "tag_d": i + 3,
    })

    AB = type("AB" + name, (A, B), {
        "ab": i ^ 0x55,
    })
    CD = type("CD" + name, (C, D), {
        "cd": i ^ 0xaa,
    })
    X = type("X" + name, (AB, CD), {
        "leaf_bias": 13 + (i & 7),
    })
    return X


def bench_dynamic_classes(n):
    checksum = 0
    i = 0
    while i < n:
        cls = make_dynamic_class(i)
        obj = cls(i)

        # 数据描述符 __set__ / __get__
        obj.desc = i + (checksum & 31)

        # 动态实例属性
        setattr(obj, "dyn_value", i ^ checksum)
        setattr(obj, "slot_" + str(i & 7), i + 3)

        # 动态类属性，反复改变类字典
        attr_name = "class_dyn_" + str(i & 7)
        setattr(cls, attr_name, i * 3 + 1)

        # MRO 元信息访问、isinstance、方法解析、描述符访问
        mro = cls.__mro__
        checksum = checksum + len(mro)
        checksum = checksum + obj.m(i)
        checksum = checksum + obj.n(i)
        checksum = checksum + getattr(obj, "dyn_value")
        checksum = checksum + getattr(cls, attr_name)
        checksum = checksum + getattr(obj, "slot_" + str(i & 7))
        if isinstance(obj, mro[2]):
            checksum = checksum ^ (i << 1)
        if issubclass(cls, mro[-2]):
            checksum = checksum + 17

        checksum = checksum & 0x7fffffffffffffff
        i = i + 1
    return checksum


def bench_small_int(n):
    # 主要保持在 31/63 bit 附近，覆盖加减乘、位运算、比较、分支。
    x = 123456789
    y = 987654321
    checksum = 0
    i = 0
    while i < n:
        x = (x * 1103515245 + 12345 + i) & 0x7fffffff
        y = ((y ^ x) + (i << 1) - 97) & 0x7fffffff

        z = (x + y + i) & 0x7fffffff
        z = z ^ ((z << 7) & 0x7fffffff)
        z = z - (z >> 3)

        if (z & 15) == 7:
            checksum = checksum ^ (z >> 2)
        else:
            checksum = checksum + (z & 255)

        checksum = checksum & 0x7fffffffffffffff
        i = i + 1
    return checksum


def bench_big_int(n):
    # 明显超过 64 bit，覆盖大整数加减乘、移位、取模、整除、异或。
    mask = (1 << 521) - 1
    mod = (1 << 257) - 93
    big_const = (1 << 400) + (1 << 177) + 123456789123456789
    x = (1 << 390) + 0x123456789abcdef
    checksum = 0
    i = 0
    while i < n:
        x = (x * 6364136223846793005 + (i << 129) + big_const) & mask
        y = (x * x + (i << 240) + big_const) & mask
        q = y // mod
        r = y % mod
        x = ((r << 113) ^ q ^ (x >> 29) ^ i) & mask

        if (x & 3) == 1:
            checksum = checksum + ((x >> 240) & 0xffffffff)
        else:
            checksum = checksum ^ ((x >> 311) & 0xffffffff)

        checksum = checksum & ((1 << 127) - 1)
        i = i + 1
    return checksum


def func_a(a, b):
    return (a + b) ^ ((a << 3) & 0x7fffffff)


def func_b(a, b, c=17):
    return ((a * 33) + (b ^ c) - (a >> 5)) & 0x7fffffff


def func_c(a, b, c=5, d=9):
    return (a + b * c - d + ((a ^ b) & 255)) & 0x7fffffff


def func_seq(a, b, rest):
    total = a + b
    j = 0
    n = len(rest)
    while j < n:
        total = total + rest[j]
        j = j + 1
    return total & 0x7fffffff


def make_closure(k):
    def inner(x, y=3):
        return (x + y + k + ((x ^ k) & 127)) & 0x7fffffff
    return inner


def tiny_rec(depth, value):
    if depth <= 0:
        return (value + 1) & 0x7fffffff
    return (tiny_rec(depth - 1, value + depth) + depth) & 0x7fffffff


def bench_functions(n):
    # 覆盖普通函数、默认参数、闭包、函数对象列表分派、数组参数、少量递归。
    funcs = [func_a, func_b, func_c]
    clos = make_closure(123)
    extra_args = [1, 2, 3, 4]

    x = 1
    checksum = 0
    i = 0
    while i < n:
        fn = funcs[i % 3]
        x = fn(x, i)
        x = clos(x)

        if (i & 31) == 0:
            x = func_seq(x, i, extra_args)

        if (i & 127) == 0:
            x = tiny_rec(4, x)

        checksum = checksum + x + (i & 255)
        checksum = checksum & 0x7fffffffffffffff
        i = i + 1
    return checksum


def bench_containers(n):
    # 覆盖 list/tuple/dict/set/bytearray 的创建、索引、写入、查询、切片、append/pop。
    size = 4096
    mask = size - 1

    arr = [((i * 17) ^ (i << 2)) & 0xffff for i in range(size)]
    tup = tuple(arr)
    keys = ["k" + str(i) for i in range(size)]

    d = {}
    st = set()
    i = 0
    while i < size:
        d[i] = arr[i]
        d[keys[i]] = i
        st.add(arr[i])
        i = i + 1

    buf = bytearray(size)
    stack = []
    checksum = 0
    j = 0
    i = 0

    while i < n:
        j = (j * 1103515245 + 12345 + i) & mask

        a = arr[j]
        b = arr[(j + 17) & mask]
        c = tup[(j + 31) & mask]

        val = (a + b + c + i) & 0xffff
        arr[j] = val
        buf[j] = val & 255

        d[j] = val
        checksum = checksum + d.get(j, 0)

        k = keys[j]
        if k in d:
            checksum = checksum ^ d[k]

        if val in st:
            checksum = checksum + 3
        else:
            st.add(val)
            checksum = checksum + 7

        stack.append(val)
        if len(stack) > 128:
            checksum = checksum + stack.pop()

        if (i & 1023) == 0:
            # 少量切片，避免只测索引。
            part = arr[(j - 8) & mask:j & mask]
            checksum = checksum + len(part)

        if (i & 2047) == 0:
            old_key = (j + 97) & mask
            checksum = checksum + d.pop(old_key, 0)
            d[old_key] = old_key ^ val

        checksum = checksum & 0x7fffffffffffffff
        i = i + 1

    return checksum + len(d) + len(st) + len(stack) + buf[0]


def bench_text_exception(n):
    # 覆盖字符串拼接、比较、startswith/endswith、split、join，以及低频异常路径。
    words = ["alpha", "beta", "gamma", "delta", "epsilon", "zeta", "theta", "lambda"]
    checksum = 0
    i = 0
    while i < n:
        w = words[i & 7]
        s = w + "_" + str(i & 1023) + "_" + words[(i >> 3) & 7]

        if s.startswith("a"):
            checksum = checksum + len(s)
        elif s.endswith("a"):
            checksum = checksum + 3
        else:
            checksum = checksum + (len(s) & 7)

        if (i & 255) == 0:
            parts = s.split("_")
            s2 = ":".join(parts)
            checksum = checksum + len(s2)

        try:
            if (i & 4095) == 2047:
                raise ValueError("planned benchmark exception")
            checksum = checksum ^ (ord(s[0]) << 1)
        except ValueError as exc:
            checksum = checksum + len(str(exc)) + i

        checksum = checksum & 0x7fffffffffffffff
        i = i + 1
    return checksum


def run_all():
    print("pure_python_aot_bench")
    print("no import, no f-string")
    print("SCALE", SCALE)

    total = 0

    r = bench_dynamic_classes(DYNAMIC_CLASS_ROUNDS)
    print("dynamic_classes", DYNAMIC_CLASS_ROUNDS, r)
    total = total ^ r

    r = bench_small_int(SMALL_INT_ROUNDS)
    print("small_int", SMALL_INT_ROUNDS, r)
    total = total ^ r

    r = bench_big_int(BIG_INT_ROUNDS)
    print("big_int", BIG_INT_ROUNDS, r)
    total = total ^ r

    r = bench_functions(FUNCTION_ROUNDS)
    print("functions", FUNCTION_ROUNDS, r)
    total = total ^ r

    r = bench_containers(CONTAINER_ROUNDS)
    print("containers", CONTAINER_ROUNDS, r)
    total = total ^ r

    r = bench_text_exception(TEXT_EXCEPTION_ROUNDS)
    print("text_exception", TEXT_EXCEPTION_ROUNDS, r)
    total = total ^ r

    print("final_checksum", total)


if __name__ == "__main__":
    run_all()
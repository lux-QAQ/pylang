class BadTuple:
    def __init__(self, items):
        self.items = list(items)

    def __hash__(self):
        # 模拟 tuple 的 hash（依赖内容）
        return hash(tuple(self.items))

    def __eq__(self, other):
        return isinstance(other, BadTuple) and self.items == other.items

    def __repr__(self):
        return "BadTuple(" + str(self.items) + ")"

    # ❗ 提供“可变性”
    def set(self, index, value):
        self.items[index] = value


# 创建对象
t = BadTuple([1, 2])

# 放入 dict
d = {t: "original"}

print("初始:", d)

# 修改内容（模拟“可变 tuple”）
t.set(0, 100)

print("修改后对象:", t)

#尝试访问
print("尝试访问 d[t]:", d.get(t))

#构造一个“内容相同”的新对象
t2 = BadTuple([100, 2])
print("新对象:", t2)
print("尝试访问 d[t2]:", d.get(t2))

# 查看 dict 内部状态
print("dict keys:", list(d.keys()))
print(BadTuple)
# 4×4矩阵分块乘法顺序示例

## 设定

A = 4×4，B = 4×4，C = 4×4

A、B、C元素编号如下：

A:
| a0 | a1 | a2 | a3 |
|----|----|----|----|
| a4 | a5 | a6 | a7 |
| a8 | a9 | a10| a11|
| a12| a13| a14| a15|

B:
| b0 | b1 | b2 | b3 |
|----|----|----|----|
| b4 | b5 | b6 | b7 |
| b8 | b9 | b10| b11|
| b12| b13| b14| b15|

C:
| c0 | c1 | c2 | c3 |
|----|----|----|----|
| c4 | c5 | c6 | c7 |
| c8 | c9 | c10| c11|
| c12| c13| c14| c15|

## 1️⃣ 块级ijk（for ii for jj for kk）

内层：for i in ii, for j in jj, for k in kk

顺序（以ii=0,jj=0,kk=0为例，块大小2）：

```
for ii in [0,2]:
  for jj in [0,2]:
    for kk in [0,2]:
      for i in [ii,ii+1]:
        for j in [jj,jj+1]:
          for k in [kk,kk+1]:
            c[i*4+j] += a[i*4+k] * b[k*4+j]
```

### 具体16步（ii=0,jj=0,kk=0）

1. c0 += a0*b0
2. c0 += a1*b4
3. c1 += a0*b1
4. c1 += a1*b5
5. c4 += a4*b0
6. c4 += a5*b4
7. c5 += a4*b1
8. c5 += a5*b5
9. c0 += a0*b2
10. c0 += a2*b8
11. c1 += a0*b3
12. c1 += a2*b9
13. c4 += a4*b2
14. c4 += a6*b8
15. c5 += a4*b3
16. c5 += a6*b9

## 2️⃣ 块级kij（for kk for ii for jj）

```
for kk in [0,2]:
  for ii in [0,2]:
    for jj in [0,2]:
      for i in [ii,ii+1]:
        for j in [jj,jj+1]:
          for k in [kk,kk+1]:
            c[i*4+j] += a[i*4+k] * b[k*4+j]
```

### 具体16步（kk=0,ii=0,jj=0）

1. c0 += a0*b0
2. c0 += a1*b4
3. c1 += a0*b1
4. c1 += a1*b5
5. c4 += a4*b0
6. c4 += a5*b4
7. c5 += a4*b1
8. c5 += a5*b5
9. c0 += a0*b2
10. c0 += a2*b8
11. c1 += a0*b3
12. c1 += a2*b9
13. c4 += a4*b2
14. c4 += a6*b8
15. c5 += a4*b3
16. c5 += a6*b9

## 3️⃣ 块级jki（for jj for kk for ii）

```
for jj in [0,2]:
  for kk in [0,2]:
    for ii in [0,2]:
      for i in [ii,ii+1]:
        for j in [jj,jj+1]:
          for k in [kk,kk+1]:
            c[i*4+j] += a[i*4+k] * b[k*4+j]
```

### 具体16步（jj=0,kk=0,ii=0）

1. c0 += a0*b0
2. c0 += a1*b4
3. c1 += a0*b1
4. c1 += a1*b5
5. c4 += a4*b0
6. c4 += a5*b4
7. c5 += a4*b1
8. c5 += a5*b5
9. c0 += a0*b2
10. c0 += a2*b8
11. c1 += a0*b3
12. c1 += a2*b9
13. c4 += a4*b2
14. c4 += a6*b8
15. c5 += a4*b3
16. c5 += a6*b9

---

## 图示

A:
| a0 | a1 | a2 | a3 |
|----|----|----|----|
| a4 | a5 | a6 | a7 |
| a8 | a9 | a10| a11|
| a12| a13| a14| a15|

B:
| b0 | b1 | b2 | b3 |
|----|----|----|----|
| b4 | b5 | b6 | b7 |
| b8 | b9 | b10| b11|
| b12| b13| b14| b15|

C:
| c0 | c1 | c2 | c3 |
|----|----|----|----|
| c4 | c5 | c6 | c7 |
| c8 | c9 | c10| c11|
| c12| c13| c14| c15|

---

> 你可以对比发现：只要内层顺序一样，三种块级的每个小块内的16步**完全一样**，但外层循环决定了块的加载/驱逐顺序。

---

如需其它块级顺序（ikj、jik、kji）也可补充。

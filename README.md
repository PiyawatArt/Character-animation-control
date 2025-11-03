# Hero Controller (OpenGL + Mixamo)

โปรเจกต์เกมจำลองการควบคุมตัวละคร 3D  
พัฒนาโดยใช้ **C++ / OpenGL / GLFW / GLM**  
ตัวละครและแอนิเมชันทั้งหมดนำมาจาก **Mixamo**

---

## Features

- เดิน (Walk) / วิ่ง (Run) / ถอยหลัง (Backward)
- ต่อย (Punch) และ เตะ (Kick)
- กระโดด (Jump) 1 ครั้งต่อการกด Space 1 ครั้ง
- สลับมุมกล้องได้ (Follow Camera / Free Camera)
- กล้องติดตามตัวละคร หรือควบคุมอิสระด้วยเมาส์และปุ่มลูกศร
- พื้นหลังและพื้นมีเอฟเฟกต์กริดและหมอกด้วย Shader
- ระบบอนิเมชันด้วย Animator และ Animation class จาก LearnOpenGL

---

## Controls

| Key | Action |
|-----|---------|
| W | เดินไปข้างหน้า |
| Shift + W | วิ่งไปข้างหน้า |
| S | เดินถอยหลัง |
| A / D | หมุนซ้าย / หมุนขวา |
| Q | ต่อย |
| E | เตะ |
| Space | กระโดด (เล่นท่า jump.dae หนึ่งครั้ง) |
| C | เปลี่ยนมุมกล้อง Follow ↔ Free |
| ESC | ออกจากเกม |

---

## Technical Stack

- OpenGL 3.3 Core Profile  
- GLFW — จัดการหน้าต่างและอินพุต  
- GLAD — โหลดฟังก์ชัน OpenGL  
- GLM — คณิตศาสตร์ 3D  
- stb_image — โหลด texture  
- Custom Shaders — สำหรับพื้นและพื้นหลัง  
- Animator / Model Loader — จาก LearnOpenGL  

---

## Assets

ไฟล์โมเดลและแอนิเมชันทั้งหมดนำมาจาก [Mixamo](https://www.mixamo.com)

ใช้ไฟล์ `.dae` ดังนี้:
```
hero.dae
Idle.dae
walk.dae
run.dae
backward.dae
punch.dae
kick.dae
jump.dae
```

---

## Credit

- Character & Animation: Mixamo  
- Animation System: LearnOpenGL  

---

## Demo Video


https://github.com/user-attachments/assets/14dd832e-27de-4f7b-85d7-ac88f991c301



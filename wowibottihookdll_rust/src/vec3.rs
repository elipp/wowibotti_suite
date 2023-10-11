use std::{
    f32::consts::PI,
    ops::{Add, Div, Mul, Sub},
};

pub const TWO_PI: f32 = PI * 2.0;

#[derive(Debug, Copy, Clone, Default)]
pub struct Vec3 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

impl std::fmt::Display for Vec3 {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Vec3({:.3}, {:.3}, {:.3})", self.x, self.y, self.z)
    }
}

impl Add for Vec3 {
    type Output = Self;

    fn add(self, b: Self) -> Self {
        Self {
            x: self.x + b.x,
            y: self.y + b.y,
            z: self.z + b.z,
        }
    }
}

impl Sub for Vec3 {
    type Output = Self;

    fn sub(self, b: Self) -> Self {
        Self {
            x: self.x - b.x,
            y: self.y - b.y,
            z: self.z - b.z,
        }
    }
}

impl Mul<f32> for Vec3 {
    type Output = Self;

    fn mul(self, b: f32) -> Self {
        Self {
            x: b * self.x,
            y: b * self.y,
            z: b * self.z,
        }
    }
}

impl Div<f32> for Vec3 {
    type Output = Self;

    fn div(self, b: f32) -> Self {
        let inv = b.recip();
        Self {
            x: inv * self.x,
            y: inv * self.y,
            z: inv * self.z,
        }
    }
}

impl Mul<Vec3> for f32 {
    type Output = Vec3;

    fn mul(self, rhs: Vec3) -> Self::Output {
        rhs * self
    }
}

impl Vec3 {
    pub fn length(&self) -> f32 {
        (self.x * self.x + self.y * self.y + self.z * self.z).sqrt()
    }
    pub fn unit(&self) -> Self {
        *self / (self.length().max(0.01))
    }
    pub fn from_rot_value(rot: f32) -> Self {
        Self {
            x: rot.cos(),
            y: rot.sin(),
            z: 0.0,
        }
    }
    pub fn to_rot_value(self) -> f32 {
        // assumes this is an unit direction vector
        let angle = self.y.atan2(self.x);
        if angle < 0.0 {
            angle + 2.0 * PI
        } else {
            angle
        }
    }
    pub fn dot(a: Self, b: Self) -> f32 {
        a.x * b.x + a.y * b.y + a.z * b.z
    }
    pub fn rotated_2d_cw(self, angle: f32) -> Self {
        let sin = angle.sin();
        let cos = angle.cos();
        Self {
            x: self.x * cos - self.y * sin,
            y: self.x * sin + self.y * cos,
            z: 0.0,
        }
    }

    pub fn zero_z(self) -> Self {
        Self {
            x: self.x,
            y: self.y,
            z: 0.0,
        }
    }

    pub fn select_longer(a: Vec3, b: Vec3) -> Vec3 {
        if a.length() > b.length() {
            a
        } else {
            b
        }
    }

    pub fn select_shorter(a: Vec3, b: Vec3) -> Vec3 {
        if a.length() < b.length() {
            a
        } else {
            b
        }
    }

    pub fn select_closer(&self, a: Vec3, b: Vec3) -> Vec3 {
        if (*self - a).length() < (*self - b).length() {
            a
        } else {
            b
        }
    }
}

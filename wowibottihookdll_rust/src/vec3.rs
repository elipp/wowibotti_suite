use std::{
    f32::consts::PI,
    ops::{Add, Div, Mul, Neg, Sub},
};

use serde::{Deserialize, Serialize};

pub const TWO_PI: f32 = PI * 2.0;

#[derive(Debug, Copy, Clone, Default, Serialize, Deserialize)]
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

impl Neg for Vec3 {
    type Output = Self;

    fn neg(self) -> Self {
        Self {
            x: -self.x,
            y: -self.y,
            z: -self.z,
        }
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

const ALPHA: f32 = 0.5; // "centripetal"

pub struct CatmullRom {
    t0: f32,
    t1: f32,
    t2: f32,
    t3: f32,
    p0: Vec3,
    p1: Vec3,
    p2: Vec3,
    p3: Vec3,
}

impl CatmullRom {
    pub fn new(p0: Vec3, p1: Vec3, p2: Vec3, p3: Vec3) -> Self {
        let t0 = 0.0;
        let t1 = t0 + (p1 - p0).length().powf(ALPHA);
        let t2 = t1 + (p2 - p1).length().powf(ALPHA);
        let t3 = t2 + (p3 - p2).length().powf(ALPHA);
        Self {
            t0,
            t1,
            t2,
            t3,
            p0,
            p1,
            p2,
            p3,
        }
    }
    pub fn sample(&self, t: f32) -> (Vec3, Vec3) {
        // let's hope the compiler makes this optimal xD
        let (t0, t1, t2, t3) = (self.t0, self.t1, self.t2, self.t3);
        let (p0, p1, p2, p3) = (self.p0, self.p1, self.p2, self.p3);

        let t = t1 + (t2 - t1) * t; // lerp

        let a1 = (t1 - t) / (t1 - t0) * p0 + (t - t0) / (t1 - t0) * p1;
        let a2 = (t2 - t) / (t2 - t1) * p1 + (t - t1) / (t2 - t1) * p2;
        let a3 = (t3 - t) / (t3 - t2) * p2 + (t - t2) / (t3 - t2) * p3;

        let b1 = (t2 - t) / (t2 - t0) * a1 + (t - t0) / (t2 - t0) * a2;
        let b2 = (t3 - t) / (t3 - t1) * a2 + (t - t1) / (t3 - t1) * a3;

        let c = (t2 - t) / (t2 - t1) * b1 + (t - t1) / (t2 - t1) * b2;

        // derivative portion
        let da1 = 1.0 / (t1 - t0) * (p1 - p0);
        let da2 = 1.0 / (t2 - t1) * (p2 - p1);
        let da3 = 1.0 / (t3 - t2) * (p3 - p2);

        let db1 =
            1.0 / (t2 - t0) * (a2 - a1) + (t2 - t) / (t2 - t0) * da1 + (t - t0) / (t2 - t0) * da2;
        let db2 =
            1.0 / (t3 - t1) * (a3 - a2) + (t3 - t) / (t3 - t1) * da2 + (t - t1) / (t3 - t1) * da3;

        let dc =
            1.0 / (t2 - t1) * (b2 - b1) + (t2 - t) / (t2 - t1) * db1 + (t - t1) / (t2 - t1) * db2;

        (c, dc)
    }
}

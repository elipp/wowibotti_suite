use std::ops::{
    Add, AddAssign, Deref, Div, DivAssign, Index, IndexMut, Mul, MulAssign, Neg, Sub, SubAssign,
};

use nalgebra::{Vector3, Vector4};

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WowVector3(pub Vector3<f32>);

// ---------------------------------------------------------------------------
// Arithmetic with WowVector3
// ---------------------------------------------------------------------------

impl Add for WowVector3 {
    type Output = Self;
    fn add(self, rhs: Self) -> Self {
        WowVector3(self.0 + rhs.0)
    }
}

impl AddAssign for WowVector3 {
    fn add_assign(&mut self, rhs: Self) {
        self.0 += rhs.0;
    }
}

impl Sub for WowVector3 {
    type Output = Self;
    fn sub(self, rhs: Self) -> Self {
        WowVector3(self.0 - rhs.0)
    }
}

impl SubAssign for WowVector3 {
    fn sub_assign(&mut self, rhs: Self) {
        self.0 -= rhs.0;
    }
}

impl Neg for WowVector3 {
    type Output = Self;
    fn neg(self) -> Self {
        WowVector3(-self.0)
    }
}

// ---------------------------------------------------------------------------
// Scalar arithmetic (f32)
// ---------------------------------------------------------------------------

impl Mul<f32> for WowVector3 {
    type Output = Self;
    fn mul(self, rhs: f32) -> Self {
        WowVector3(self.0 * rhs)
    }
}

impl Mul<WowVector3> for f32 {
    type Output = WowVector3;
    fn mul(self, rhs: WowVector3) -> WowVector3 {
        WowVector3(self * rhs.0)
    }
}

impl MulAssign<f32> for WowVector3 {
    fn mul_assign(&mut self, rhs: f32) {
        self.0 *= rhs;
    }
}

impl Div<f32> for WowVector3 {
    type Output = Self;
    fn div(self, rhs: f32) -> Self {
        WowVector3(self.0 / rhs)
    }
}

impl DivAssign<f32> for WowVector3 {
    fn div_assign(&mut self, rhs: f32) {
        self.0 /= rhs;
    }
}

// ---------------------------------------------------------------------------
// Interop with raw Vector3<f32>
// ---------------------------------------------------------------------------

impl Add<Vector3<f32>> for WowVector3 {
    type Output = Self;
    fn add(self, rhs: Vector3<f32>) -> Self {
        WowVector3(self.0 + rhs)
    }
}

impl Sub<Vector3<f32>> for WowVector3 {
    type Output = Self;
    fn sub(self, rhs: Vector3<f32>) -> Self {
        WowVector3(self.0 - rhs)
    }
}

// ---------------------------------------------------------------------------
// Indexing
// ---------------------------------------------------------------------------

impl Index<usize> for WowVector3 {
    type Output = f32;
    fn index(&self, i: usize) -> &f32 {
        &self.0[i]
    }
}

impl IndexMut<usize> for WowVector3 {
    fn index_mut(&mut self, i: usize) -> &mut f32 {
        &mut self.0[i]
    }
}

impl WowVector3 {
    pub fn new(x: f32, y: f32, z: f32) -> Self {
        WowVector3(Vector3::new(x, y, z))
    }

    pub fn zero() -> Self {
        WowVector3(Vector3::zeros())
    }

    pub fn dot(self, rhs: Self) -> f32 {
        self.0.dot(&rhs.0)
    }

    pub fn cross(self, rhs: Self) -> Self {
        WowVector3(self.0.cross(&rhs.0))
    }

    pub fn norm(self) -> f32 {
        self.0.norm()
    }

    pub fn norm_squared(self) -> f32 {
        self.0.norm_squared()
    }

    pub fn normalize(self) -> Self {
        WowVector3(self.0.normalize())
    }

    pub fn lerp(self, rhs: Self, t: f32) -> Self {
        WowVector3(self.0.lerp(&rhs.0, t))
    }

    pub fn component_mul(self, rhs: Self) -> Self {
        WowVector3(self.0.component_mul(&rhs.0))
    }

    pub fn abs(self) -> Self {
        WowVector3(self.0.abs())
    }

    pub fn xyzw(self) -> WowVector4 {
        WowVector4::new(self.0.x, self.0.y, self.0.z, 1.0)
    }

    pub fn length(&self) -> f32 {
        self.0.norm()
    }

    pub fn unit(&self) -> Self {
        *self / self.length().max(0.01)
    }

    pub fn from_rot_value(rot: f32) -> Self {
        WowVector3(Vector3::new(rot.cos(), rot.sin(), 0.0))
    }

    pub fn to_rot_value(self) -> f32 {
        self.0.y.atan2(self.0.x).rem_euclid(std::f32::consts::TAU)
    }

    pub fn rotated_2d_cw(self, angle: f32) -> Self {
        let (sin, cos) = angle.sin_cos();
        WowVector3(Vector3::new(
            self.0.x * cos - self.0.y * sin,
            self.0.x * sin + self.0.y * cos,
            0.0,
        ))
    }

    pub fn zero_z(self) -> Self {
        WowVector3(Vector3::new(self.0.x, self.0.y, 0.0))
    }

    pub fn select_longer(a: Self, b: Self) -> Self {
        if a.length() > b.length() { a } else { b }
    }

    pub fn select_shorter(a: Self, b: Self) -> Self {
        if a.length() < b.length() { a } else { b }
    }

    pub fn select_closer(&self, a: Self, b: Self) -> Self {
        if (*self - a).length() < (*self - b).length() {
            a
        } else {
            b
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WowVector4(pub Vector4<f32>);

impl Deref for WowVector4 {
    type Target = Vector4<f32>;
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

// ---------------------------------------------------------------------------
// WoW -> nalgebra  (wow2glm)
//   x' = -y,  y' = z,  z' = -x
// ---------------------------------------------------------------------------

impl From<WowVector3> for Vector3<f32> {
    fn from(WowVector3(v): WowVector3) -> Self {
        Vector3::new(-v.y, v.z, -v.x)
    }
}

impl From<WowVector4> for Vector4<f32> {
    fn from(WowVector4(v): WowVector4) -> Self {
        Vector4::new(-v.y, v.z, -v.x, v.w)
    }
}

// ---------------------------------------------------------------------------
// nalgebra -> WoW  (glm2wow)
//   x' = -z,  y' = -x,  z' = y
// ---------------------------------------------------------------------------

impl From<Vector3<f32>> for WowVector3 {
    fn from(v: Vector3<f32>) -> Self {
        WowVector3(Vector3::new(-v.z, -v.x, v.y))
    }
}

impl From<Vector4<f32>> for WowVector4 {
    fn from(v: Vector4<f32>) -> Self {
        WowVector4(Vector4::new(-v.z, -v.x, v.y, v.w))
    }
}

impl std::ops::DerefMut for WowVector4 {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl Add for WowVector4 {
    type Output = Self;
    fn add(self, rhs: Self) -> Self {
        WowVector4(self.0 + rhs.0)
    }
}

impl AddAssign for WowVector4 {
    fn add_assign(&mut self, rhs: Self) {
        self.0 += rhs.0;
    }
}

impl Sub for WowVector4 {
    type Output = Self;
    fn sub(self, rhs: Self) -> Self {
        WowVector4(self.0 - rhs.0)
    }
}

impl SubAssign for WowVector4 {
    fn sub_assign(&mut self, rhs: Self) {
        self.0 -= rhs.0;
    }
}

impl Neg for WowVector4 {
    type Output = Self;
    fn neg(self) -> Self {
        WowVector4(-self.0)
    }
}

impl Mul<f32> for WowVector4 {
    type Output = Self;
    fn mul(self, rhs: f32) -> Self {
        WowVector4(self.0 * rhs)
    }
}

impl Mul<WowVector4> for f32 {
    type Output = WowVector4;
    fn mul(self, rhs: WowVector4) -> WowVector4 {
        WowVector4(self * rhs.0)
    }
}

impl MulAssign<f32> for WowVector4 {
    fn mul_assign(&mut self, rhs: f32) {
        self.0 *= rhs;
    }
}

impl Div<f32> for WowVector4 {
    type Output = Self;
    fn div(self, rhs: f32) -> Self {
        WowVector4(self.0 / rhs)
    }
}

impl DivAssign<f32> for WowVector4 {
    fn div_assign(&mut self, rhs: f32) {
        self.0 /= rhs;
    }
}

impl Add<Vector4<f32>> for WowVector4 {
    type Output = Self;
    fn add(self, rhs: Vector4<f32>) -> Self {
        WowVector4(self.0 + rhs)
    }
}

impl Sub<Vector4<f32>> for WowVector4 {
    type Output = Self;
    fn sub(self, rhs: Vector4<f32>) -> Self {
        WowVector4(self.0 - rhs)
    }
}

impl Index<usize> for WowVector4 {
    type Output = f32;
    fn index(&self, i: usize) -> &f32 {
        &self.0[i]
    }
}

impl IndexMut<usize> for WowVector4 {
    fn index_mut(&mut self, i: usize) -> &mut f32 {
        &mut self.0[i]
    }
}

impl std::fmt::Display for WowVector4 {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "({:.3}, {:.3}, {:.3}, {:.3})",
            self.0.x, self.0.y, self.0.z, self.0.w
        )
    }
}

impl WowVector4 {
    pub fn new(x: f32, y: f32, z: f32, w: f32) -> Self {
        WowVector4(Vector4::new(x, y, z, w))
    }

    pub fn zero() -> Self {
        WowVector4(Vector4::zeros())
    }

    pub fn dot(self, rhs: Self) -> f32 {
        self.0.dot(&rhs.0)
    }

    pub fn norm(self) -> f32 {
        self.0.norm()
    }

    pub fn norm_squared(self) -> f32 {
        self.0.norm_squared()
    }

    pub fn normalize(self) -> Self {
        WowVector4(self.0.normalize())
    }

    pub fn lerp(self, rhs: Self, t: f32) -> Self {
        WowVector4(self.0.lerp(&rhs.0, t))
    }

    pub fn component_mul(self, rhs: Self) -> Self {
        WowVector4(self.0.component_mul(&rhs.0))
    }

    pub fn abs(self) -> Self {
        WowVector4(self.0.abs())
    }

    /// Drops w and returns the xyz part as a WowVector3.
    pub fn xyz(self) -> WowVector3 {
        WowVector3(self.0.xyz())
    }
}

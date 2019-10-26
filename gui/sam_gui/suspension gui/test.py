import math
import numpy as np
from sympy import Symbol, nsolve
import sympy
import mpmath

class point:
	def __init__(self, x, y, z):
		self.x = x
		self.y = y
		self.z = z

	def array(self):
		return np.array([self.x, self.y, self.z])

	def tuple(self):
		return (self.x, self.y, self.z)

# 14.393 = ((x-7.999)^2 + (y-5.947)^2 + (z-65.151)^2) 
# 14.489 = ((x-7.999)^2 + (y-5.947)^2 + (z-55.370)^2) 

point1 = point(7.999, 5.947, 65.151)
point2 = point(7.999, 5.947, 55.370)
len1 = 14.393
len2 = 14.489
y = 6.25
guess = point(21.582, 6.25, 60.402)

mpmath.mp.dps = 15
x = Symbol('x')
z = Symbol('z')
f1 = ((x - point1.x)**2 + (y - point1.y)**2 + (z - point1.z)**2) - len1**2
f2 = ((x - point2.x)**2 + (y - point2.y)**2 + (z - point2.z)**2) - len2**2
print (nsolve((f1, f2), (x, z), (guess.x, guess.z)))
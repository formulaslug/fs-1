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

def caster(self, upper, lower):
	return math.atan2(lower.y - upper.y, lower.z - upper.z)

def length(p1, p2):
	return np.sqrt(((p2.x - p1.x)**2) + ((p2.y - p1.y)**2) + ((p2.z - p1.z)**2))

def triang_2(p1, p2, len1, len2, z, guess):
	x = Symbol('x')
	y = Symbol('y')
	f1 = ((x - p1.x)**2 + (y - p1.y)**2 + (z - p1.z)**2) - len1**2
	f2 = ((x - p2.x)**2 + (y - p2.y)**2 + (z - p2.z)**2) - len2**2
	try:
		return nsolve((f1, f2), (x, y), (guess.x, guess.y))
	except ValueError:
		print("you dingus")
		return guess.array()

def triang_3(p1, p2, p3, len1, len2, len3, guess):
	x = Symbol('x')
	y = Symbol('y')
	z = Symbol('z')
	f1 = ((x - p1.x)**2 + (y - p1.y)**2 + (z - p1.z)**2) - len1**2
	f2 = ((x - p2.x)**2 + (y - p2.y)**2 + (z - p2.z)**2) - len2**2
	f3 = ((x - p3.x)**2 + (y - p3.y)**2 + (z - p3.z)**2) - len3**2
	try:
		return nsolve((f1, f2, f3), (x, y, z), (guess.x, guess.y, guess.z))
	except ValueError:
		print("you dingus")
		return guess.array()

def angle(p1, p2):
	return atan2(p2.x-p1.x, p2.y-p1.y)

def kingpin(upper, lower):
	return math.degrees(math.atan2(upper.x-lower.x, upper.z-lower.z))

def caster(upper, lower):
	return math.degrees(math.atan2(upper.y-lower.y, upper.z-lower.z))
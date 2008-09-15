import pygame

class Box:

	x = 0
	y = 0

	def setPosition(self, x, y):
		self.x = x
		self.y = y

	def draw(self, screen):
		gray = (64, 64, 64)
		lightgray = (90, 90, 90)

		pos = (self.x * screen.get_width(), self.y * screen.get_height())

		# Draw a circle
		pygame.draw.circle(screen, lightgray, pos, 70, 10)

		# And the box
		rect = (pos, (50, 50))
		pygame.draw.rect(screen, gray, rect)


import pygame
from pygame.locals import *
import noya.gfx
import tuio
import clutter


class Application:

	wg = []

	def __init__(self):
		stage = clutter.Stage()
		stage.set_size(500,500)
		stage.set_color(clutter.color_parse('DarkSlateGray'))

	def run(self):

		stage.show()
		stage.connect("key-press-event", clutter.main_quit)
		clutter.main()

		self.tracking = tuio.Tracking()
		print "loaded profiles:", self.tracking.profiles.keys()
		print "list functions to access tracked objects:", self.tracking.get_helpers()


		# Create objects
		self.wg.append(noya.gfx.Box())

		# Draw
		for object in self.wg:
			object.draw(self.screen)

		pygame.display.update()

		try:

			while 1:
				self.tracking.update()

				self.screen.fill((10,10,10))

				for obj in self.tracking.objects():
					g = noya.gfx.Box()
					g.setPosition(obj.xpos, obj.ypos)
					g.draw(self.screen)

				pygame.display.update()

		except KeyboardInterrupt:
			self.tracking.stop()

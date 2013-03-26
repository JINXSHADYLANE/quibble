if stage == 0 then
	if mage_talking == 0 then
		dialog_text = {
			{ "Strange woman: ", "Good day, young boy. How do you like secrets? \
			Maybe you want to know one?" },
			{ "You: ", "Not at all... I'm not in the mood."},
			{ "Strange woman: ", "I see..." },
			{ "Strange woman: ", "You know, I've heard there is a magical book, \
			which fulfills your biggest dream! I think you should look for it." }
		}
		mage_talking = 1
	elseif mage_talking == 1 then
		dialog_text = {
			{ "Strange woman: ", "Have you forget my little secret? Just go \
			and look for a magical book." }
		}
	end
end



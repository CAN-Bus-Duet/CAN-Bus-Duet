// For RAID defense

inline unsigned long padID(unsigned long idA){
	unsigned long idExt = 0;
	bool randomize = true;
	if (randomize){
		unsigned long idB = random(1UL << 18);
		idExt = ((idA << 18) | idB);
	} else {
		idExt = idA << 18;
	}
	return idExt;
}

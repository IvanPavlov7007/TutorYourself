#pragma once
#include <QVariant>

class ProbabilityRandom
{
public:
	ProbabilityRandom() = default;
	void addOccurrence(QVariant value, int probability);
	QVariant Next();
private:
	QList< QPair<QVariant, int> > occurrences;
	int totalOfPossibleOutcomes = 0;
};
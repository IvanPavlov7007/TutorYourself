#include "probabilityrandom.h"

void ProbabilityRandom::addOccurrence(QVariant value, int probability)
{
	if (probability > 0)
	{
		occurrences.append(QPair<QVariant, int>(value,probability));
		totalOfPossibleOutcomes += probability;
	}
}

QVariant ProbabilityRandom::Next()
{
	if (occurrences.count() == 0)
		return QVariant();
	int occurrence = qrand() % totalOfPossibleOutcomes;
	int checkedPartOfAllOutcomes = 0;

	QListIterator<QPair<QVariant, int> > i(occurrences);
	while (i.hasNext())
	{
		QPair<QVariant, int> pair = i.next();
		if (occurrence < checkedPartOfAllOutcomes + pair.second && occurrence >= checkedPartOfAllOutcomes)
			return pair.first;
		checkedPartOfAllOutcomes += pair.second;
	}
	return QVariant();
}

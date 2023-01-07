#!/bin/env python3

from json import dump
from faker import Faker
from sys import stdout

faker = Faker()

ARRAY_ELEM = 100
OBJECT_ELEM = 100

element = {}
for j in range(OBJECT_ELEM):
    element[faker.name()] = faker.text()
element["array"] = [k for k in range(ARRAY_ELEM)]

result = []

for i in range(ARRAY_ELEM):
    result.append(element)


dump(result, stdout)

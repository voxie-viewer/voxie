import abc
import json

from . import json_util


types = {}


symbols = [
    "H", "He", "Li", "Be", "B", "C", "N", "O", "F", "Ne", "Na", "Mg",
    "Al", "Si", "P", "S", "Cl", "Ar", "K", "Ca", "Sc", "Ti", "V", "Cr",
    "Mn", "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br", "Kr",
    "Rb", "Sr", "Y", "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd",
    "In", "Sn", "Sb", "Te", "I", "Xe", "Cs", "Ba", "La", "Ce", "Pr", "Nd",
    "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb", "Lu", "Hf",
    "Ta", "W", "Re", "Os", "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi", "Po",
    "At", "Rn", "Fr", "Ra", "Ac", "Th", "Pa", "U", "Np", "Pu", "Am", "Cm",
    "Bk", "Cf", "Es", "Fm", "Md", "No", "Lr", "Rf", "Db", "Sg", "Bh", "Hs",
    "Mt", "Ds", "Rg", "Cn", "Nh", "Fl", "Mc", "Lv", "Ts", "Og",
]


def chemical_composition_type_impl(cls):
    name = cls.chemical_composition_type

    if name in types:
        raise Exception('Duplicate chemical composition type: {!r}'.format(name))
    types[name] = cls

    return cls


def parse(json_data):
    json_array = json_util.expect_array(json_data)
    if len(json_data) == 0:
        raise Exception('Got empty array in ChemicalComposition')
    ty = json_util.expect_string(json_data[0])
    if ty in types:
        return types[ty].parse(json_array)
    raise Exception('Invalid ChemicalComposition type: {!r}'.format(ty))


def num_to_string(number, digits):
    # if number % 1 == 0:
    #     s = str(round(number))
    # else:
    s = str(number)

    res = ''
    for c in s:
        if c >= '0' and c <= '9':
            res += digits[ord(c) - ord('0')]
        elif c == '.':
            res += digits[10]
        else:
            res += c

    return res


class ChemicalComposition(abc.ABC):
    @abc.abstractmethod
    def to_json(self):
        ...


@chemical_composition_type_impl
class ChemicalCompositionElement(ChemicalComposition):
    chemical_composition_type = 'element'

    def __init__(self, atomic_number, nucleon_number):
        self.atomic_number = atomic_number
        self.nucleon_number = nucleon_number

    @staticmethod
    def parse(json_array):
        if len(json_array) < 2:
            raise Exception('Not enough values in array for ChemicalCompositionElement')

        atomic_number = json_util.expect_int(json_array[1])

        nucleon_number = None
        if len(json_array) >= 3 and json_array[2] is not None:
            nucleon_number = json_util.expect_int(json_array[2])

        return ChemicalCompositionElement(atomic_number=atomic_number, nucleon_number=nucleon_number)

    def to_json(self):
        res = [ChemicalCompositionElement.chemical_composition_type, self.atomic_number]
        if self.nucleon_number is not None:
            res.append(self.nucleon_number)
        return res

    def __repr__(self):
        if self.atomic_number < 1:
            raise Exception('atomic_number < 1')
        pos = self.atomic_number - 1
        if pos >= len(symbols):
            raise Exception('atomic_number too large')
        symbol = symbols[pos]

        if self.nucleon_number is not None:
            # https://stackoverflow.com/questions/34350441/is-there-an-unicode-symbol-for-superscript-comma
            # Using U+00B7 Middle Dot
            return num_to_string(self.nucleon_number, ["⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹", "·"]) + symbol
        else:
            return symbol


class Member:
    def __init__(self, fraction, composition):
        self.fraction = fraction
        self.composition = composition

        if type(fraction) != int and type(fraction) != float:
            raise Exception("type(fraction) != int and type(fraction) != float")

    def to_json(self):
        return [self.fraction, self.composition.to_json()]


@chemical_composition_type_impl
class ChemicalCompositionMolecule(ChemicalComposition):
    chemical_composition_type = 'molecule'

    def __init__(self, members):
        self.members = members

    @staticmethod
    def parse(json_array):
        if len(json_array) < 2:
            raise Exception('Not enough values in array for ChemicalCompositionMolecule')

        members_json = json_util.expect_array(json_array[1])
        members = []
        for member_json in members_json:
            member_array = json_util.expect_array(member_json)
            if len(member_array) < 2:
                raise Exception('Not enough values in array for Member')
            fraction = json_util.expect_number(member_array[0])
            composition = parse(member_array[1])
            member = Member(fraction=fraction, composition=composition)
            members.append(member)

        return ChemicalCompositionMolecule(members=members)

    def to_json(self):
        members_json = []
        for member in self.members:
            members_json.append(member.to_json())
        return [ChemicalCompositionMolecule.chemical_composition_type, members_json]

    def __repr__(self):
        res = ''
        for member in self.members:
            if isinstance(member.composition, ChemicalCompositionElement):
                res += str(member.composition)
            elif isinstance(member.composition, ChemicalCompositionMolecule):
                res += "(" + str(member.composition) + ")"
            else:
                raise Exception('Invalid member in ChemicalCompositionMolecule.__repr__()')
            if member.fraction != 1:
                res += num_to_string(member.fraction, ["₀", "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉", "."])
        return res


@chemical_composition_type_impl
class ChemicalCompositionCompound(ChemicalComposition):
    chemical_composition_type = 'compound'

    def __init__(self, compound_type_str, members):
        self.compound_type_str = compound_type_str
        self.members = members

    @staticmethod
    def parse(json_array):
        if len(json_array) < 3:
            raise Exception('Not enough values in array for ChemicalCompositionCompound')

        compound_type_str = json_util.expect_string(json_array[1])

        members_json = json_util.expect_array(json_array[2])
        members = []
        for member_json in members_json:
            member_array = json_util.expect_array(member_json)
            if len(member_array) < 2:
                raise Exception('Not enough values in array for Member')
            fraction = json_util.expect_number(member_array[0])
            composition = parse(member_array[1])
            member = Member(fraction=fraction, composition=composition)
            members.append(member)

        return ChemicalCompositionCompound(compound_type_str=compound_type_str, members=members)

    def to_json(self):
        members_json = []
        for member in self.members:
            members_json.append(member.to_json())
        return [ChemicalCompositionCompound.chemical_composition_type, self.compound_type, members_json]

    def __repr__(self):
        res = ''
        res += 'Compound ' + self.compound_type_str + '['
        first = True
        for member in self.members:
            if not first:
                res += " + "
            if member.fraction != 1:
                res += str(member.fraction)
            res += str(member.composition)
            first = False
        res += ']'
        return res


@chemical_composition_type_impl
class ChemicalCompositionMeta(ChemicalComposition):
    chemical_composition_type = 'meta'

    def __init__(self, member, information):
        self.member = member
        self.information = information

    @staticmethod
    def parse(json_array):
        if len(json_array) < 3:
            raise Exception('Not enough values in array for ChemicalCompositionMeta')

        member = parse(json_array[1])

        information = json_util.expect_object(json_array[2])

        return ChemicalCompositionMeta(member=member, information=information)

    def to_json(self):
        members_json = []
        for member in self.members:
            members_json.append(member.to_json())
        return [ChemicalCompositionMeta.chemical_composition_type, self.meta_type, members_json]

    def __repr__(self):
        return '<' + str(self.member) + '> ' + json.dumps(self.information, indent=2, ensure_ascii=False, allow_nan=False)

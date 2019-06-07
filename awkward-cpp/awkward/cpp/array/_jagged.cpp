#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <cinttypes>
#include <stdexcept>
namespace py = pybind11;

struct JaggedArraySrc {
public:

    template <typename T>
    static py::array_t<T> offsets2parents(py::array_t<T> offsets) {
        py::buffer_info offsets_info = offsets.request();
        if (offsets_info.size <= 0) {
            throw std::invalid_argument("offsets must have at least one element");
        }
        auto offsets_ptr = (T*)offsets_info.ptr;

        size_t parents_length = (size_t)offsets_ptr[offsets_info.size - 1];
        auto parents = py::array_t<T>(parents_length);
        py::buffer_info parents_info = parents.request();

        auto parents_ptr = (T*)parents_info.ptr;

        size_t j = 0;
        size_t k = -1;
        for (size_t i = 0; i < (size_t)offsets_info.size; i++) {
            while (j < (size_t)offsets_ptr[i]) {
                parents_ptr[j] = k;
                j += 1;
            }
            k += 1;
        }

        return parents;
    }

    template <typename T>
    static py::array_t<T> counts2offsets(py::array_t<T> counts) {
        py::buffer_info counts_info = counts.request();
        auto counts_ptr = (T*)counts_info.ptr;

        size_t offsets_length = counts_info.size + 1;
        auto offsets = py::array_t<T>(offsets_length);
        py::buffer_info offsets_info = offsets.request();
        auto offsets_ptr = (T*)offsets_info.ptr;

        offsets_ptr[0] = 0;
        for (size_t i = 0; i < (size_t)counts_info.size; i++) {
            offsets_ptr[i + 1] = offsets_ptr[i] + counts_ptr[i];
        }
        return offsets;
    }

    template <typename T>
    static py::array_t<T> startsstops2parents(py::array_t<T> starts, py::array_t<T> stops) {
        py::buffer_info starts_info = starts.request();
        auto starts_ptr = (T*)starts_info.ptr;

        py::buffer_info stops_info = stops.request();
        auto stops_ptr = (T*)stops_info.ptr;

        size_t max;
        if (stops_info.size < 1) {
            max = 0;
        }
        else {
            max = (size_t)stops_ptr[0];
            for (size_t i = 1; i < (size_t)stops_info.size; i++) {
                if ((size_t)stops_ptr[i] > max) {
                    max = (size_t)stops_ptr[i];
                }
            }
        }
        auto parents = py::array_t<T>(max);
        py::buffer_info parents_info = parents.request();
        auto parents_ptr = (T*)parents_info.ptr;
        for (size_t i = 0; i < max; i++) {
            parents_ptr[i] = -1;
        }

        for (size_t i = 0; i < (size_t)starts_info.size; i++) {
            for (size_t j = (size_t)starts_ptr[i]; j < (size_t)stops_ptr[i]; j++) {
                parents_ptr[j] = i;
            }
        }

        return parents;
    }

    template <typename T>
    static py::tuple parents2startsstops(py::array_t<T> parents, T length = -1) {
        py::buffer_info parents_info = parents.request();
        auto parents_ptr = (T*)parents_info.ptr;

        if (length < 0) {
            length = 0;
            for (size_t i = 0; i < (size_t)parents_info.size; i++) {
                if (parents_ptr[i] > length) {
                    length = parents_ptr[i];
                }
            }
            length++;
        }

        auto starts = py::array_t<T>((size_t)length);
        py::buffer_info starts_info = starts.request();
        auto starts_ptr = (T*)starts_info.ptr;

        auto stops = py::array_t<T>((size_t)length);
        py::buffer_info stops_info = stops.request();
        auto stops_ptr = (T*)stops_info.ptr;

        for (size_t i = 0; i < (size_t)length; i++) {
            starts_ptr[i] = 0;
            stops_ptr[i] = 0;
        }

        T last = -1;
        for (size_t k = 0; k < (size_t)parents_info.size; k++) {
            auto thisOne = parents_ptr[k];
            if (last != thisOne) {
                if (last >= 0 && last < length) {
                    stops_ptr[last] = (T)k;
                }
                if (thisOne >= 0 && thisOne < length) {
                    starts_ptr[thisOne] = (T)k;
                }
            }
            last = thisOne;
        }

        if (last != -1) {
            stops_ptr[last] = (T)parents_info.size;
        }

        py::list temp;
        temp.append(starts);
        temp.append(stops);
        py::tuple out(temp);
        return out;
    }

    template <typename T>
    static py::tuple uniques2offsetsparents(py::array_t<T> uniques) {
        py::buffer_info uniques_info = uniques.request();
        auto uniques_ptr = (T*)uniques_info.ptr;

        size_t tempLength;
        if (uniques_info.size < 1) {
            tempLength = 0;
        }
        else {
            tempLength = uniques_info.size - 1;
        }
        auto tempArray = py::array_t<bool>(tempLength);
        py::buffer_info tempArray_info = tempArray.request();
        auto tempArray_ptr = (bool*)tempArray_info.ptr;

        size_t countLength = 0;
        for (size_t i = 0; i < (size_t)uniques_info.size - 1; i++) {
            if (uniques_ptr[i] != uniques_ptr[i + 1]) {
                tempArray_ptr[i] = true;
                countLength++;
            }
            else {
                tempArray_ptr[i] = false;
            }
        }
        auto changes = py::array_t<T>(countLength);
        py::buffer_info changes_info = changes.request();
        auto changes_ptr = (T*)changes_info.ptr;
        size_t index = 0;
        for (size_t i = 0; i < (size_t)tempArray_info.size; i++) {
            if (tempArray_ptr[i]) {
                changes_ptr[index++] = (T)(i + 1);
            }
        }

        auto offsets = py::array_t<T>(changes_info.size + 2);
        py::buffer_info offsets_info = offsets.request();
        auto offsets_ptr = (T*)offsets_info.ptr;
        offsets_ptr[0] = 0;
        offsets_ptr[offsets_info.size - 1] = (T)uniques_info.size;
        for (size_t i = 1; i < (size_t)offsets_info.size - 1; i++) {
            offsets_ptr[i] = changes_ptr[i - 1];
        }

        auto parents = py::array_t<T>(uniques_info.size);
        py::buffer_info parents_info = parents.request();
        auto parents_ptr = (T*)parents_info.ptr;
        for (size_t i = 0; i < (size_t)parents_info.size; i++) {
            parents_ptr[i] = 0;
        }
        for (size_t i = 0; i < (size_t)changes_info.size; i++) {
            parents_ptr[(size_t)changes_ptr[i]] = 1;
        }
        for (size_t i = 1; i < (size_t)parents_info.size; i++) {
            parents_ptr[i] += parents_ptr[i - 1];
        }

        py::list temp;
        temp.append(offsets);
        temp.append(parents);
        py::tuple out(temp);
        return out;
    }

};

PYBIND11_MODULE(_jagged, m) {
    py::class_<JaggedArraySrc>(m, "JaggedArraySrc")
        .def(py::init<>())
        .def_static("offsets2parents", &JaggedArraySrc::offsets2parents<std::int64_t>)
        .def_static("offsets2parents", &JaggedArraySrc::offsets2parents<std::int32_t>)
        .def_static("counts2offsets", &JaggedArraySrc::counts2offsets<std::int64_t>)
        .def_static("counts2offsets", &JaggedArraySrc::counts2offsets<std::int32_t>)
        .def_static("startsstops2parents", &JaggedArraySrc::startsstops2parents<std::int64_t>)
        .def_static("startsstops2parents", &JaggedArraySrc::startsstops2parents<std::int32_t>)
        .def_static("parents2startsstops", &JaggedArraySrc::parents2startsstops<std::int64_t>)
        .def_static("parents2startsstops", &JaggedArraySrc::parents2startsstops<std::int32_t>)
        .def_static("uniques2offsetsparents", &JaggedArraySrc::uniques2offsetsparents<std::int64_t>)
        .def_static("uniques2offsetsparents", &JaggedArraySrc::uniques2offsetsparents<std::int32_t>);
}

# PimPam

This is the official implementation of PimPam, the first graph pattern matching framework implemented on real-world, commercially available PIM hardware platforms UPMEM.

# Usage
To match a pattern within a graph, run the following command:
```
GRAPH=<graph_name> PATTERN=<pattern_name> make test
```

To specify a data graph, e.g., LiveJournal, add the following line to ``include/common.h`` (See line 41-44):
```
#if defined(LJ)
#define DATA_NAME "soc-LiveJournall"
#define N (1<<23)
#define M (1<<27)
```
Here, ``LJ`` is used as the ``graph_name``. And the data file should be named as ``./data/${DATA_NAME}.bin``. ``N`` and ``M`` are the number of nodes and edges, rounded up to the nearest power of 2. PimPam only accepts binary CSR format, i.e., the first four bytes specifying the number of nodes $n$, followed by the number of edges $m$ (also four bytes). After that, there should be an array indicating ``row_ptr`` and an array indicating ``col_idx``, with $n$ and $m$ elements, respectively.

The choice for ``pattern_name`` includes ``CLIQUE3,CLIQUE4,CYCLE4,HOUSE5,TRI_TRI6``, which we have implemented. You may also implement your own pattern. To do this, add the following line to ``include/common.h`` (See line 70-72):
```
if defined(CLIQUE3)
#define KERNEL_FUNC clique3
#define PATTERN_NAME "clique3"
```
Here, ``CLIQUE3`` is used as ``pattern_name``. Also, create a new file ``CLIQUE3.c`` in ``dpu/`` and implement the following function:
```
void clique3(sysname_t tasklet_id);
```
See ``dpu/CLIQUE3.c`` for an example.

# Citation
If you find PimPam useful in your research, please cite our paper:
```
@article{pimpam_sigmod24,
  title   = {{PimPam: Efficient Graph Pattern Matching on Real Processing-in-Memory Hardware}},
  author  = {Shuangyu Cai and Boyu Tian and Huanchen Zhang and Mingyu Gao},
  journal = {Proceedings of the ACM on Management of Data (SIGMOD)},
  volume  = {2},
  number  = {3},
  year    = {2024},
  month   = {Jun},
  pages   = {161:1-161:25}
}
```

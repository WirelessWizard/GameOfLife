[make_general_stat.sh]
bash-скрипт, который многократно запускает скрипт stat.sh, подставляя различные параметры (количество поколений) с заданным шагом.
Запустите скрипт без параметров для получения справки.

[stat.sh]
bash-скрипт, позволяющий автоматически собирать статистику по производительности mpi-алгоритма.
Предполагается, что скрипт stat.sh лежит в директории mpi_progs. Кроме того, там же есть три другие директории: sum_factorial, merge_sort, thermal_conductivity, содержащие исходные тексты всех трех программ (файлы main.c) и вспомогательные файлы, а также несколько gnuplot-скриптов для визуализации результатов.
Для получения статистики, после сборки всех проектов командами mpicc main.c, нужно вызвать скрипт stat.sh из директории mpi_progs.
Скрипт принимает несколько параметров:
    ./stat.sh  MAX_CORES  STAT_BASE  STAT_DATA_MASK  MPI_BINARY_PROG_NAME [arg1 arg2 ...]
MAX_CORES -- максимальное число процессоров, для которых вызывается mpirun и собирается статистика;
STAT_BASE -- объем выборки, по которой происходит статистическое усреднение;
STAT_DATA_MASK -- тексторое описание в выводе программы, который отвечает за показатель производительности, например строка "Computation time" для первой программы;
MPI_BINARY_PROG_NAME [arg1 arg2 ...] -- путь из директории mpi_progs к исполняемому файлу и аргументы. В данном случае "./[sum_factorial|merge_sort|thermal_conductivity]/a.out [...]".

Результатом работы скрипта является вывод усредненных производительностей для каждого числа процессоров в stdout и файл с префиксом stat__ и расширением .dat, пригодный для визуализации с помощью команды autoplot.sh . Этот скрипт автоматически ищет файлы с расширением .dat в данной директории, запускает gnuplot, получает postscript и конвертирует итоговые графики в растровый формат (PNG). 

Примеры запуска скрипта:
                                  Ядер:  Вычислений:
[s91612@hex mpi_progs]$ ./stat.sh    16           10 "Computation time" sum_factorial/a.out 14
[s91612@hex mpi_progs]$ ./stat.sh    16            4 Acceleration merge_sort/a.out merge_sort/pi_data.txt

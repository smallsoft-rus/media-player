v2.5 (19.02.2023)
- Добавлена поддержка версий информационных тегов ID3v2.2 и ID3v2.4
- Добавлена поддержка отображения встроенных обложек альбомов из тегов ID3v2.3 и ID3v2.4
- Добавлена поддержка комментариев ID3v2 в кодировках UTF8 и UTF16
- Добавлена возможность отображения URL из тегов ID3v2
- Добавлена возможность отображения информации о версии модуля для фильтров DirectShow при отсутствии у них страницы свойств
- Добавлено открытие файла изображения во внешней программе при двойном щелчке на обложке альбома (только для обложек из файлов)
- Исправлено отображение форматов аудио ADPCM, Monkey's Audio (APE) и AMR NB/WB в информации о форматах мультимедиа

v2.4 (02.05.2022)
- Теперь для воспроизведения файла проигрыватель сначала пытается использовать режим Media Foundation, а если файл воспроизвести в нем невозможно, использует DirectShow (ранее было наоборот)
- Добавлена возможность просмотра информации о форматах мультимедиа в режиме Media Foundation
- Переработан регулятор громкости, так чтобы изменение уровня громкости в режимах Media Foundation и DirectShow происходило одинаково
- Исправлены проблемы с производительностью при перемотке на большой промежуток времени в режиме Media Foundation
- Исправлено блокирование воспроизведения модальным диалоговым окном при возникновении ошибки в режиме Media Foundation
- Исправлена ошибка Access violation при завершении воспроизведения файла в режиме Media Foundation
- Исправлена некорректная перерисовка окна видео во время паузы
- Исправлена ошибка при попытке воспроизвести видеофайл на машине, не имеющей ни одного активного звукового устройства

v2.3 (07.07.2021)
- Добавлена поддержка декодеров, использующих технологию Media Foundation
- Убрана поддержка Windows XP
- Убрана поддержка воспроизведения Audio CD
- Исправлено некорректное положение контекстного меню при щелчке правой кнопкой мыши по заголовкам столбцов в списке воспроизведения

v2.2 (18.05.2021)
- Исправлено аварийное завершение программы при попытке открыть файл с пустыми тегами ID3v2

v2.1 (25.04.2021)
- Исправлена ошибка, из-за которой иногда некорректно отображалась информация из тегов при использовании "Открыть с помощью".
- Исправлена ошибка, при которой информация из тегов некорректно обрабатывалась при наличии символа "~" в тегах.
- Исправлено мерцание индикатора текущей позиции.
- Списки воспроизведения теперь сохраняются и считываются в кодировке UTF-8 (ранее использовалась текущая кодовая страница ANSI). Это позволяет корректно обрабатывать пути файлов с национальными символами из других кодовых страниц.
- Исправлено неровное положение некоторых элементов в главном окне.
- Реализована плавная (логарифмическая) регулировка громкости.
- При повторном запуске программы теперь выполняется попытка вывести окно главного экземпляра на передний план.
- Обновлен алгоритм поиска изображения по умолчанию, теперь изображение берется из каталога стандартных обоев Windows (`%WINDIR%\Web\Wallpaper\Windows\`).
- Ошибки воспроизведения теперь выводятся в отдельное немодальное окно и не блокируют воспроизведение (проигрыватель автоматически переходит к следующему файлу).
- Добавлено логирование ошибок (файл `(Документы)\Small Media Player\error.log`)

v2.0 (19.09.2017)
- Добавлена поддержка Windows 10
- Изменен интерфейс, теперь список воспроизведения и обложка автоматически изменяют размер при изменении размеров главного окна
- В главном окне добавлена краткая информация о форматах аудио и видео в воспроизводимом файле
- Добавлено выделение стрелкой текущего файла в списке воспроизведения
- Опция отображения фонового рисунка заменена на аналогичную опцию отображения рисунка при отсутствии обложки
- Настройки программы и список воспроизведения теперь хранятся в папке "Мои документы"
- Если включено запоминание позиции, запоминается также громкость.
- m4a добавлен в список обрабатываемых расширений
- Программа теперь собрана с помощью Visual C++ 2012

v1.6 (1.02.2012)
- поддержка информационных тегов ID3V2, APE, FLAC
- отображение значков в списке воспроизведения, в зависимости от типа файла
- изменение последовательности элементов списка с помощью контекстного меню (Вырезать - Вставить)
- сортировка списка воспроизведения по щелчку на заголовке столбца
- функция сворачивания в значок системной панели
- поддержка клавиш мультимедийной клавиатуры

v1.5 (18.08.2011)
- загрузка и сохранение списков воспроизведения в формате M3U
- отображение обложек альбомов из файлов JPEG, BMP, PNG
- запоминание позиции последнего воспроизведенного файла при закрытии
- отображение фонового рисунка в окне программы
- используется стандартный системный диалог "Выбор папки"
- небольшие усовершенствования в интерфейсе
- возможность отображения страниц свойств декодеров

v1.4 (2011 г.)
- регулятор громкости
- возможность развернуть окно видео на полный экран
- режимы воспроизведения: повтор, случайный порядок
- управление клавишами
- возможность скрыть список воспроизведения

v1.3
- в списке воспроизведения отображается дополнительная информация из тэгов ID3v1
- в ассоциациях файлов изменены команды
- файлы из папки добавляются в список воспроизведения отсортированными по алфавиту
- улучшено управление списком воспроизведения

v1.2
- добавлено воспроизведение AudioCD (только WinXP)
- улучшенная схема выбора декодера для форматов MP3, AVI

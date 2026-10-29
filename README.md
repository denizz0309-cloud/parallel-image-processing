\# C++ ve OpenCV ile Paralel Görüntü İşleme



Bu proje, C++ ve OpenCV kullanarak görüntü kuantizasyonu, çok iş parçacıklı parlaklık ayarlama, paralel histogram hesaplama ve piksel değerleri üzerinde matematiksel işlemler gerçekleştirmektedir.



\## Projenin Amacı



Bir şehir fotoğrafını gri tonlamalı görüntüye dönüştürerek altı farklı görüntü işleme işlemini gerçekleştirmek ve tek iş parçacığı (single thread) ile dört iş parçacığı (4 threads) kullanılarak yapılan işlemlerin çalışma sürelerini karşılaştırmak.



\## Gerçekleştirilen İşlemler



1\. \*\*8 bitten 6 bite kuantizasyon:\*\* Gri tonlamalı görüntüdeki piksel değerleri 0–255 aralığından 64 farklı parlaklık seviyesine (0–63) dönüştürülür. Görüntüyü kaydedebilmek için değerler tekrar 8 bitlik biçimde saklanır.

2\. \*\*Dört bölgede parlaklık ayarlama:\*\* Görüntü dört bölgeye ayrılır. Her bölgeye sırasıyla -60, -20, +20 ve +60 parlaklık farkı uygulanır.

3\. \*\*Parlaklık işlemlerinin süre karşılaştırması:\*\* Parlaklık ayarlama işlemi tek iş parçacığıyla ve dört iş parçacığıyla 30 kez çalıştırılır. Ortalama çalışma süreleri karşılaştırılır.

4\. \*\*Paralel histogram hesaplama:\*\* Görüntünün dört bölgesinin histogramı dört ayrı iş parçacığında hesaplanır ve sonuçlar birleştirilerek 256 değerli ana histogram oluşturulur.

5\. \*\*Histogram sürelerinin karşılaştırılması:\*\* Histogram hesaplama işleminin tek iş parçacığı ve dört iş parçacığı ile çalışma süreleri ölçülür. İki yöntemin histogram sonuçlarının aynı olup olmadığı kontrol edilir.

6\. \*\*Piksel değerleri üzerinde matematiksel işlemler:\*\* Piksel değerlerine ayrı ayrı `piksel × 0.75` ve `piksel × 0.75 + 20` işlemleri uygulanır. Oluşan görüntüler karşılaştırılır ve ortalama mutlak piksel farkı hesaplanır.



\## Kullanılan Teknolojiler



\- C++

\- OpenCV 4.14.0

\- C++17

\- `std::thread` — çok iş parçacıklı programlama

\- `std::chrono` — çalışma süresi ölçümü

\- Visual Studio 2022

\- Windows



\## Girdi Görüntüsü



Programın çalışabilmesi için `city.jpg` dosyası proje çalışma klasöründe bulunmalıdır.



\## Oluşturulan Çıktılar



Program, sonuçları `results` klasörüne kaydeder.



| Dosya adı | Açıklama |

|---|---|

| `01\_grayscale.png` | Gri tonlamalı görüntü |

| `02\_quantized\_6bit.png` | 6 bit parlaklık seviyelerine kuantize edilmiş görüntü |

| `03\_brightness\_four\_regions.png` | Dört bölgeye farklı parlaklık değerleri uygulanmış görüntü |

| `04\_histogram.png` | Gri tonlamalı görüntünün histogramı |

| `05\_multiply\_075.png` | Piksel değerlerinin 0.75 ile çarpıldığı görüntü |

| `06\_multiply\_075\_plus\_20.png` | Piksel değerlerinin 0.75 ile çarpılıp 20 eklendiği görüntü |

| `07\_difference.png` | İki matematiksel işlem arasındaki mutlak fark görüntüsü |



\## Performans Analizi



Program, her performans testini 30 kez çalıştırarak ortalama süreyi milisaniye (ms) cinsinden hesaplar.



Dört iş parçacığı kullanılması her zaman daha hızlı sonuç alınacağı anlamına gelmez. İş parçacıklarının oluşturulma maliyeti, görüntünün boyutu, işlemci özellikleri ve bellek erişimi performansı etkileyebilir.



Ayrıca program, tek iş parçacığıyla ve dört iş parçacığıyla elde edilen parlaklık sonuçlarının aynı olup olmadığını ve histogramların birebir eşleşip eşleşmediğini kontrol eder.



\## Programı Çalıştırma



1\. OpenCV kurulumunu yapın ve Visual Studio içerisinde gerekli başlık dosyası, kütüphane ve DLL yollarını tanımlayın.

2\. `ParallelImageProcessing.sln` dosyasını Visual Studio ile açın.

3\. Yapılandırmayı \*\*Release\*\*, platformu \*\*x64\*\* olarak seçin.

4\. Projeyi derleyin ve çalıştırın.

5\. Konsoldaki performans ölçümlerini ve doğrulama sonuçlarını inceleyin.

6\. Görüntü çıktılarını `results` klasöründen açın.



\## Teknik Notlar



\- 6 bitlik kuantizasyon sonucunda 64 farklı parlaklık seviyesi elde edilir. Görüntü dosyası, uyumluluk amacıyla 8 bitlik formatta kaydedilir.

\- Parlaklık ayarlamasında piksel değerleri 0–255 aralığının dışına çıkamaz; taşan değerler bu aralığa sınırlandırılır.

\- Çalışma süreleri bilgisayara ve her çalıştırmadaki koşullara göre değişebilir.




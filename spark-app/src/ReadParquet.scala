import org.apache.spark.sql.SparkSession
import java.sql.Timestamp
import org.apache.log4j.{Level, Logger}


object ReadParquet {

  Logger.getLogger("org").setLevel(Level.WARN)
  Logger.getLogger("akka").setLevel(Level.WARN)

  def main(args: Array[String]): Unit = {

    val file = "spark-app/resources/example/parquet/userdata/userdata1.parquet"
    val spark = SparkSession.builder
      .master("local")
      .appName("ReadParquet")
      .getOrCreate()
    import spark.implicits._

    val userdata = spark.read.parquet(file)

    userdata.filter(row => row.getTimestamp(0).after(Timestamp.valueOf("2016-02-03 00:00:00"))).show()

  }
}
